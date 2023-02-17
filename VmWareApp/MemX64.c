// mem_x64.c : 实现了x64/IA32e/长模式分页/内存模型。.
// 作      者: 学技术打豆豆
//
#include "MemX64.h"
#include "VmWareApp.h"
#include "vad.h"

#define PTE_SWIZZLE_MASK                               0x0         //Todo:暂时不支持
#define PTE_SWIZZLE_BIT                                0x10        // nt!_MMPTE_SOFTWARE.SwizzleBit
#define MEM_X64_PTE_IS_HARDWARE(pte)                  (pte & 0x01)
#define MEM_X64_MEMMAP_DISPLAYBUFFER_LINE_LENGTH      89

#define MEM_X64_PTE_PAGE_FILE_NUMBER(pte)          ((pte >> 12) & 0x0f)
#define MEM_X64_PTE_PAGE_FILE_OFFSET(pte)          ((pte >> 32) ^ (!(pte & PTE_SWIZZLE_BIT) ? PTE_SWIZZLE_MASK : 0))

#define MEM_X64_PTE_IS_FILE_BACKED(pte)            (((pte & 0xffff80000000000f) == 0xffff800000000000))

/*
nt!_MMPTE_PROTOTYPE
+ 0x000 Valid            : Pos 0, 1 Bit
+ 0x000 DemandFillProto : Pos 1, 1 Bit
+ 0x000 HiberVerifyConverted : Pos 2, 1 Bit
+ 0x000 ReadOnly : Pos 3, 1 Bit
+ 0x000 SwizzleBit : Pos 4, 1 Bit
+ 0x000 Protection : Pos 5, 5 Bits
+ 0x000 Prototype : Pos 10, 1 Bit
+ 0x000 Combined : Pos 11, 1 Bit
+ 0x000 Unused1 : Pos 12, 4 Bits
+ 0x000 ProtoAddress : Pos 16, 48 Bits
*/
#define MEM_X64_PTE_PROTOTYPE(pte)                    (((pte & 0x8000000000070401) == 0x8000000000000400) ? ((pte >> 16) | 0xffff000000000000) : 0)

/*
nt!_MMPTE_TRANSITION
+ 0x000 Valid            : Pos 0, 1 Bit
+ 0x000 Write : Pos 1, 1 Bit
+ 0x000 Spare : Pos 2, 1 Bit
+ 0x000 IoTracker : Pos 3, 1 Bit
+ 0x000 SwizzleBit : Pos 4, 1 Bit
+ 0x000 Protection : Pos 5, 5 Bits
+ 0x000 Prototype : Pos 10, 1 Bit
+ 0x000 Transition : Pos 11, 1 Bit
+ 0x000 PageFrameNumber : Pos 12, 36 Bits
+ 0x000 Unused : Pos 48, 16 Bits
*/
#define MEM_X64_PTE_TRANSITION(pte)                   (((pte & 0x0c01) == 0x0800) ? ((pte & 0xffffdffffffff000) | 0x005) : 0)
#define MEM_X64_PTE_IS_TRANSITION(H, pte, iPML)       ((((pte & 0x0c01) == 0x0800) && (iPML == 1) && (H->vmm.tpSystem == VMM_SYSTEM_WINDOWS_X64)) ? ((pte & 0xffffdffffffff000) | 0x005) : 0)
#define MEM_X64_PTE_IS_VALID(pte, iPML)               (pte & 0x01)


DWORD64 MemX64Prototype(_In_ DWORD64 pte,_In_ DWORD64 DirectoryTableBase)
{
	DWORD64 PtePage = 0;
	do
	{
		if (!pte || !DirectoryTableBase){
			break;
		}
		if (!VMReadVmVirtualAddr(&PtePage, DirectoryTableBase, MEM_X64_PTE_PROTOTYPE(pte), 8)) {
			break;
		}

		if (!PtePage) {
			break;
		}

		//如果是原型PTE，则是Subsection Pte,这里暂时没有处理
		//否则就是PTE的Hard/Transition/PageFile 交给外面去处理
		if (MEM_X64_PTE_PROTOTYPE(PtePage)) {
			PtePage = 0;
			break;
		}

	} while (FALSE);
	
	return PtePage;
}

//从虚拟内存读取“分页”页面
VMM_PTE_TP MemX64TransitionPaged(_In_ DWORD64 va, _In_ DWORD64 pte, _In_ DWORD64 DirectoryTableBase, _Out_ PDWORD64 ppa)
{
	DWORD PfNumber = 0;
	DWORD PfOffset = 0;
	DWORD PteTp = VMM_PTE_TP_NA;
	do
	{
		/*
		有效PTE(硬件PTE)
		PTE设置了有效位，MMU就发挥了作用，并执行到物理地址的转换,这里就不会是分页内存
		*/
		if (MEM_X64_PTE_IS_HARDWARE(pte))
		{
			PteTp = VMM_PTE_TP_HARDWARE;
			*ppa = pte & 0x0000fffffffff000;
			break;
		}
		/*无效pte(软件PTE)--------------------------------------------------------------------------------------------------------------------------------------------
		4种无效PTE和1种特殊无效PTE(原型PTE)

		如果在地址转换过程中遇到的PTE的有效位为零，则PTE表示无效的页，在引用时将引发内存管理异常或页错误。
		MMU忽略PTE的剩余位，因此操作系统可以使用这些位来存储关于页面的信息，这将有助于解决页面错误。
		下面列出了四种无效pte及其结构。这些通常被称为软件pte，因为它们是由内存管理器而不是MMU解释的*/
		/*无效PTE有四种：
		1、在分页文件中
		2、要求0页面
		3、页面转移中
	    4、未知，需要检查VAD树
		*/	

		/* 原型状态
           1、相同的物理页面可以在许多不同的进程之间共享。这很容易做到，因为您可以只让多个 PTE 引用同一个物理页面。操作系统的问题是如何协调共享页面的修剪。
		由于有许多对同一物理页面的引用，如果操作系统需要例如将物理页面重新定位到页面文件中，它将需要搜索和更新所有这些引用。
		由于效率非常低，因此 Windows 解决方案是使用一种“符号链接”PTE 将共享页面定向到另一个 PTE - 称为原型 PTE。
		因此，我们只需要更新原型 PTE，所有引用共享内存的 PTE 都将自动更新。

		  2、与硬件PTE 不同，原型 PTE 是内部内存管理器数据结构，CPU 从不使用它来执行虚拟到物理地址转换。它们仅由 Windows 页面错误处理程序用于解决共享页面上的页面错误。

		原型PTE:原型PTE都是4KB的页面，
		根据原型PTE的描述,共享页面可处于下列6种状态之一°
		（l）活动／有效（active／valid）因为其他进程可以访问, 所以该页面位于物理内存中
		（2）转换（transltion目标页面位于内存中的待命列表或修改列表（或不位于这两 个列表中的任—个）内。
		（3）已修改不写出（modihed no write）目标页面位于内存中,且位于已修改不写出列表内）。
		（4）要求零（demandzero）目标页面应当用一个零页面来满足。
		（5）页面文件（pageFile）目标页面驻留在页面文件内
		（6）映射文件（mappedFile）。目标页面驻留在映射文件内
		*/
		if (MEM_X64_PTE_PROTOTYPE(pte)) {
			PteTp = VMM_PTE_TP_PROTOTYPE;
			pte = MemX64Prototype(pte, DirectoryTableBase);
			if (MEM_X64_PTE_IS_HARDWARE(pte)) {
				*ppa = pte & 0x0000fffffffff000;
				break;
			}
		}

	     /*过渡状态
		   Windows有一个“工作集修整器” - 一个从进程的工作集中删除页面的组件（工作集在POSIX中更广为人知的是驻留集，但本质上是进程可以访问的所有页面的集合，而不会出错。
		修整器尝试将页面删除到页面文件中，以增加系统中可用物理页面的总数。但是，不是立即将页面写入页面文件，而是首先将页面置于转换状态。
		这允许稍后将页面写入页面文件，同时在内存中仍包含有效数据，以防进程稍后需要该页面（它可以快速错误地返回到工作集中）。
		因此，转换中的页面包含有效数据，但是当进程访问它时，硬件会将pageerror分页到操作系统处理程序中，该处理程序将简单地将页面标记为有效。
		如果“转换”标志处于打开状态，而“原型”标志处于关闭状态，则页面处于“转换”状态
		*/
		if (MEM_X64_PTE_TRANSITION(pte)) {
			PteTp = VMM_PTE_TP_TRANSITION;
			pte = MEM_X64_PTE_TRANSITION(pte);
			if ((pte & 0x0000fffffffff000)) {
				*ppa = pte & 0x0000fffffffff000;
			}
			break;
		}

		//Todo:目前没有实现读写Read/Write pagefile.sys，但这里先写上
		PfNumber = MEM_X64_PTE_PAGE_FILE_NUMBER(pte);
		PfOffset = MEM_X64_PTE_PAGE_FILE_OFFSET(pte);

		/*VAD原型PTE和VAD硬件PTE

		 VAD原型PTE
		如果硬件 PTE 看起来像原型 PTE（即具有 Valid = 0，Prototype = 1），并且 ProtoAddress 等于特殊值0xFFFFFFFF0000则标记 VAD 原型。
		在这种情况下，我们必须找到与所讨论的虚拟地址相对应的VAD区域。然后，MMVAD 结构包含一系列与整个 VAD 范围相对应的 PTE。
		然后，我们计算原始虚拟地址与VAD区域的相对偏移量，以找到其相应的PTE。

		例如，假设我们尝试解析地址0x10000：
		遍历页表，我们确定VAD原型PTE（即ProtoAddress = 0xFFFFFFFF0000）。
		我们在过程VAD中搜索感兴趣的区域。假设我们找到一个从0x8000到0x20000的区域。
		此区域的_MMVAD对象有一个 FirstPrototypePte 成员（假设它指向 0xFFFF1000000）。
		因此，我们想要的 PTE 位于 （0x10000 - 0x8000） / 0x1000 + 0xFFFF1000000
		我们从该 PTE 解析物理地址。

		 Vad硬件PTE
		如果 PTE 完全为 0，则表示应咨询 VAD。当指向 PTE 的 PDE 无效（即整个页表无效）时，这种情况似乎也是如此。在这种情况下，我们需要以与上述VAD原型PTE相同的方式检查VAD。
		这种状态似乎与上述状态相同。*/

		if (va && !MEM_IS_KERNEL_ADDR_X64(va) && (!pte || (PfOffset == 0xffffffff))) {
			//目前暂未实现
			PteTp = VMM_PTE_TP_PROTOTYPE;
			VADFindVadByPte();
			break;
		}

		//如果是内核地址,并且PTE为，则直接退出
		if (!pte) {
			break;
		}

		// 要求零（demandzero）目标页面应当用一个零页面来满足[ nt!_MMPTE_SOFTWARE ]
		//（无效PTE的第2种情况和原型PTE的第4种情况）
		if (!PfNumber && !PfOffset) {
			PteTp = VMM_PTE_TP_DEMANDZERO;
			break;
		}

		//WIN11文件支持的内存(目前不支持)
		if (MEM_X64_PTE_IS_FILE_BACKED(pte)) {
			PteTp = VMM_PTE_TP_FILE_BACKED;
			break;
		}

		//接下来就是win10的压缩内存和虚拟内存(PageFile.sys)
	} while (FALSE);

	return PteTp;
}
