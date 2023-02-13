// mem_x64.c : 实现了x64/IA32e/长模式分页/内存模型。.
// 作      者: 学技术打豆豆
//
#include "MemX64.h"
#include "VmWareApp.h"

#define MEM_IS_KERNEL_ADDR_X64(va)                    (((va) & 0xffff800000000000) == 0xffff800000000000)

#define PTE_SWIZZLE_MASK                               0x0         //Todo:暂时不支持
#define PTE_SWIZZLE_BIT                                0x10        // nt!_MMPTE_SOFTWARE.SwizzleBit
#define MEM_X64_PTE_IS_HARDWARE(pte)                  (pte & 0x01)
#define MEM_X64_MEMMAP_DISPLAYBUFFER_LINE_LENGTH      89

#define MEM_X64_PTE_PAGE_FILE_NUMBER(pte)          ((pte >> 12) & 0x0f)
#define MEM_X64_PTE_PAGE_FILE_OFFSET(pte)          ((pte >> 32) ^ (!(pte & PTE_SWIZZLE_BIT) ? PTE_SWIZZLE_MASK : 0))

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
		if (!pte){
			break;
		}
		if (!VMReadVmVirtualAddr(&PtePage, DirectoryTableBase, MEM_X64_PTE_PROTOTYPE(pte), 8)) {
			break;
		}
		if ((MEM_X64_PTE_IS_HARDWARE(PtePage)) || MEM_X64_PTE_PROTOTYPE(PtePage)) {
			break;
		}

	} while (FALSE);
	
	return PtePage;
}

//从虚拟内存读取“分页”页面
BOOLEAN MemX64ReadPaged(_In_ DWORD64 va, _In_ DWORD64 pte, _In_ DWORD64 DirectoryTableBase, _Out_ PDWORD64 ppa)
{
	BOOLEAN bRet = FALSE;
	DWORD PfNumber = 0;
	DWORD PfOffset = 0;
	do
	{
		/*
		有效PTE(硬件PTE)
		PTE设置了有效位，MMU就发挥了作用，并执行到物理地址的转换,这里就不会是分页内存
		*/
		if (MEM_X64_PTE_IS_HARDWARE(pte))
		{
			*ppa = pte & 0x0000fffffffff000;
			break;
		}
		/*
		无效pte(软件PTE)
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

		/*原型PTE:原型PTE都是4KB的页面，
		根据原型PTE的描述,共享页面可处于下列6种状态之一°
		（l）活动／有效（active／valid）因为其他进程可以访问, 所以该页面位于物理内存中
		（2）转换（transltion目标页面位于内存中的待命列表或修改列表（或不位于这两 个列表中的任—个）内。
		（3）已修改不写出（modihed no write）目标页面位于内存中,且位于已修改不写出列表内）。
		（4）要求零（demandzero）目标页面应当用一个零页面来满足。
		（5）页面文件（pageFile）目标页面驻留在页面文件内
		（6）映射文件（mappedFile）。目标页面驻留在映射文件内
		*/
		if (MEM_X64_PTE_PROTOTYPE(pte)) {
			pte = MemX64Prototype(pte, DirectoryTableBase);
			if (MEM_X64_PTE_IS_HARDWARE(pte)) {
				*ppa = pte & 0x0000fffffffff000;
				break;
			}
		}

		/*
		  转换（transition）。转换位为］°所需页面位于内存中的待命、已修改或修改但
	      不写出列表中’或不位于任何列表中。换页器会从列表中移除该页面（如果位于某个列表
	      中）, 并将其加入进程工作集°由于不涉及I／O’这通常也叫作页面软错误.
	    */
		if (MEM_X64_PTE_TRANSITION(pte)) {
			pte = MEM_X64_PTE_TRANSITION(pte);
			if ((pte & 0x0000fffffffff000)) {
				*ppa = pte & 0x0000fffffffff000;
			}
			break;
		}

		//Todo:目前没有实现读写Read/Write pagefile.sys，但这里先写上
		PfNumber = MEM_X64_PTE_PAGE_FILE_NUMBER(pte);
		PfOffset = MEM_X64_PTE_PAGE_FILE_OFFSET(pte);

		//可能是vad支持的虚拟内存，va必须是3环地址（无效PTE的第4种情况）
		//或者_MMPTE_SOFTWARE.Valid=0，Prototype=1），并且 PageFileHigh 等于特殊值0xFFFFFFFF则标记VAD原型
		if (va && !MEM_IS_KERNEL_ADDR_X64(va) && (!pte || (PfOffset == 0xffffffff))) {
			//目前暂未实现
			DebugBreak();
		}

		//如果是内核地址
		if (!pte) {

		}

	} while (FALSE);

	return bRet;
}
