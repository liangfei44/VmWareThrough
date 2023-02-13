#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#include "WindowsNt.h"
#include "misc.h"
#include "VmWareApp.h"
#include "KernelModeInject.h"
#include "Win10X64Stage3.h"
#include "MemX64.h"

#define VM_X64_PTE_IS_VALID(pte)    (pte & 0x01)

#define ADDRESS_CALC(address) ((address & 0xFFF)? PAGE_SIZE-(address & 0xFFF):PAGE_SIZE) 

DWORD64 gMappedRegionBase = 0;
DWORD64 gMappedRegionSize = 0;
MEMORY_BASIC_INFORMATION gMemBaseInfo = {0};
HANDLE gVmWareProcessHandle = NULL;
DWORD gCurrentVersion = 0;
NT_PROCESS_DATA gNtProcessData = {0};
VM_PROCESS_DATA gDestProcessData = { 0 };
NT_PROCESS_OFFSET gNtProcessOffset = { 0 };

BOOLEAN VMReadPaged(PVOID buffer, DWORD64 DirectoryTableBase, DWORD64 va, DWORD64 pte, SIZE_T size);

BOOLEAN VMReadPaged(PVOID buffer, DWORD64 DirectoryTableBase, DWORD64 va, DWORD64 pte, SIZE_T size);
/*
宿主机得到客户机里的内核结构偏移。
首先说明，这是一段很丢人的代码，里面的数据来源都是根据各个WinOs的经验总结,没有官方的理论作为依据
后续有机会还是要通过PDB来得到各个偏移
*/
BOOLEAN VMGetWinX64ProcessOffset(_In_ NT_PROCESS_DATA* NtProcessData)
{
	BOOLEAN bRet = FALSE;
	BYTE SystemProcessData[VM_EPROCESS64_MAX_SIZE] = { 0 };
	BYTE TempProcessData[VM_EPROCESS64_MAX_SIZE] = { 0 };
	do
	{
          if (!(VMReadVmVirtualAddr(SystemProcessData,
                                  NtProcessData->MemoryKernelDirbase,
                                  NtProcessData->SystemProcessEprocess,
                                  VM_EPROCESS64_MAX_SIZE))) {
            break;
          }
		  //SignalState为TRUE表示此时该进程已被结束(SignalState目前是固定的)
		  if (*(PDWORD)(SystemProcessData + 0x4))
		  {
			  break;
		  }
		  //DirectoryTableBase(目前是固定的)
		  if (0xffff800000000000 & *(PDWORD64)(SystemProcessData + 0x28))
		  {
			  break;
		  }
		  gNtProcessOffset.DirectoryTableBaseOffset = 0x28;

		  //得到ImageFileNameOffset
		  for (int i = 0; i < VM_EPROCESS64_MAX_SIZE - 8; i += 8)
		  {
			  if (*(PDWORD64)(SystemProcessData + i) == 0x00006D6574737953) //system
			  {
				  gNtProcessOffset.ImageFileNameOffset = i;
				  break;
			  }
		  }
		  if (0 == gNtProcessOffset.ImageFileNameOffset)
		  {
			  break;
		  }	

		  //得到PID和ActiveProcessLinks 
		  for (int i = 0; i < VM_EPROCESS64_MAX_SIZE - 8; i += 8)
		  {
			  if (*(PDWORD64)(SystemProcessData + i)==4)
			  {
				  //这个时候PID有可能是正确，所以再进行验证
				  if (0xffff000000000000 != 
					  (0xffff000000000000 &
						  *(PDWORD64)(SystemProcessData + i + 8)))
				  {
					  //如果ActiveProcessLinks地址不对,则继续
					  continue;
				  }
				  // NextEprocessAddr=System.Eprocess.ActiveProcessLinks.Flink-Offset
				  // NextEprocessAddr.ActiveProcessLinks.Blink-offset=System.Eprocess(证明偏移对了)
				  //否则偏移是错的
				  DWORD64 TempEprocess = *(PDWORD64)(SystemProcessData + i + 8) - i - 8;
				  if (!VMReadVmVirtualAddr(TempProcessData,
					                     NtProcessData->MemoryKernelDirbase,
					                     TempEprocess,
					                     VM_EPROCESS64_MAX_SIZE))
				  {
					  continue;
				  }
				  if ((*(PDWORD64)(TempProcessData + i + 16) - i - 8) != NtProcessData->SystemProcessEprocess) {
					  continue;
				  }
				  gNtProcessOffset.UniqueProcessId = i;
				  gNtProcessOffset.ActiveProcessLinksOffset = i + 8;
				
				  //PEB和VAD暂未实现，后续用到了再实现。
				  bRet = TRUE;
			  }
		  }


	} while (FALSE);
	return bRet;
}

BOOLEAN VMFindMappedRegion() {
  unsigned char *p = NULL;

  char lpFilename[MAX_PATH + 1] = {0};
  for (p = NULL; VirtualQueryEx(gVmWareProcessHandle, p, &gMemBaseInfo,
                                sizeof(gMemBaseInfo)) == sizeof(gMemBaseInfo);
       p += gMemBaseInfo.RegionSize) {

	  if (gMemBaseInfo.BaseAddress > 0x00006fffffffffff)
	  {
		  break;
	  }
	  if ((gMemBaseInfo.BaseAddress != gMemBaseInfo.AllocationBase) || 
		  (gMemBaseInfo.RegionSize < 0x01000000) || (gMemBaseInfo.RegionSize > 0x10000000000) ||
		  ((gMemBaseInfo.State != MEM_COMMIT) || (gMemBaseInfo.Protect != PAGE_READWRITE) || (gMemBaseInfo.AllocationProtect != PAGE_READWRITE)))
	  {
		  continue;
      }

    if (GetMappedFileNameA(gVmWareProcessHandle, (void*)gMemBaseInfo.BaseAddress, lpFilename,
                           MAX_PATH)) {
      if (strstr(lpFilename, ".vmem")) {
        printf("Find the memory region at 0x%p with size 0x%llx\n",
               (void*)gMemBaseInfo.BaseAddress, gMemBaseInfo.RegionSize);
        gMappedRegionBase = (DWORD64)gMemBaseInfo.BaseAddress;
        gMappedRegionSize = gMemBaseInfo.RegionSize;
        return TRUE;
      }
    }
  }

  return FALSE;
}


BOOL VMReadHostRegion(PVOID buffer, ULONG64 addr, SIZE_T size) {
  return ReadProcessMemory(gVmWareProcessHandle,
                           (void *)((ULONG64)gMemBaseInfo.BaseAddress + addr),
                           buffer, size, NULL);
}

// uefi启动的系统，0x1000-0x100000的物理地址存了一个结构体PROCESSOR_START_BLOCK，而且都在页的开头。
//其中_KPROCESSOR_STATE中有system的cr3
//参考：http://standa-note.blogspot.com/2020/03/initializing-application-processors-on.html
//参考：http://publications.alex-ionescu.com/Recon/ReconBru%202017%20-%20Getting%20Physical%20with%20USB%20Type-C,%20Windows%2010%20RAM%20Forensics%20and%20UEFI%20Attacks.pdf
BOOLEAN VMNtKernelDataInit() {
	char buffer[0x10000];

	for (DWORD64 i = 0; i < 10; i++) {
		VMReadHostRegion(buffer, i * 0x10000, 0x10000);

		for (DWORD64 o = 0; o < 0x10000; o += 0x1000) {
			if (0x00000001000600E9 ^
				(0xffffffffffff00ff & *(DWORD64 *)(void *)(buffer + o))) //START BYTES
				continue;
			if (0xfffff80000000000 ^
				(0xfffff80000000000 & *(DWORD64 *)(void *)(buffer + o + 0x70))) //KERNEL ENTRY
				continue;
			if (0xffffff0000000fff & *(DWORD64 *)(void *)(buffer + o + 0xa0)) // PML4
				continue;
			gNtProcessData.MemoryKernelDirbase = *(DWORD64 *)(void *)(buffer + o + 0xa0);
			gNtProcessData.MemoryKernelEntry = *(DWORD64 *)(void *)(buffer + o + 0x70);

			return TRUE;

		}
	}
  
  return FALSE;
}

//虚拟地址转换物理地址
BOOLEAN VMTranslatePhyAddress(_In_ DWORD64 directoryTableBase,
                              _In_ DWORD64 virtualAddress,
	                          _Out_ PDWORD64 phyAddress) {
  BOOLEAN bRet = FALSE;
  WORD PML4 = (WORD)((virtualAddress >> 39) & 0x1FF);
  WORD DirectoryPtr = (WORD)((virtualAddress >> 30) & 0x1FF);
  WORD Directory = (WORD)((virtualAddress >> 21) & 0x1FF);
  WORD Table = (WORD)((virtualAddress >> 12) & 0x1FF);
  DWORD64 PML4E = 0;
  DWORD64 PDPTE = 0;
  DWORD64 PDE = 0;
  DWORD64 PTE = 0;
  do
  {
	  //虚拟地址检查
	if ((LONG64)virtualAddress >> 0x2F != -1 && (LONG64)virtualAddress >> 0x2F != 0){
		  break;
	}

	*phyAddress = 0;

    if (!VMReadHostRegion(&PML4E,
                          directoryTableBase + (DWORD64)PML4 * sizeof(DWORD64),
                          sizeof(PML4E))) {
      break;
    }
    if (PML4E == 0 || !VM_X64_PTE_IS_VALID(PML4E)) {
      break;
    }

	if (!VMReadHostRegion(&PDPTE,
                         (PML4E & 0xFFFF1FFFFFF000) +
                         (DWORD64)DirectoryPtr * sizeof(DWORD64),
                          sizeof(PDPTE))) {
          break;
     }
	if (PDPTE == 0){
		break;
	}

	if (!VM_X64_PTE_IS_VALID(PDPTE)) {
		*phyAddress = PDPTE;
		break;
	}

	//PDPTE.PS=1; 1G页面
	if ((PDPTE & (1 << 7)) != 0) {
		*phyAddress = (PDPTE & 0xFFFFFC0000000) + (virtualAddress & 0x3FFFFFFF);
		bRet = TRUE;
		break;
	}

	if (!VMReadHostRegion(&PDE,
                         (PDPTE & 0xFFFFFFFFFF000) +
                         (DWORD64)Directory * sizeof(DWORD64),
                          sizeof(PDE))) {
          break;
     }

	if (PDE == 0) {
		break;
	}

	if (!VM_X64_PTE_IS_VALID(PDE)) {
		*phyAddress = PDE;
		break;
	}

	//PDE.PS=1;2M页面
	if ((PDE & (1 << 7)) != 0) {
		*phyAddress = (PDE & 0xFFFFFFFE00000) + (virtualAddress & 0x1FFFFF);
		bRet = TRUE;
		break;
	}
	
	if (!VMReadHostRegion(&PTE,
                         (PDE & 0xFFFFFFFFFF000) + (DWORD64)Table * sizeof(DWORD64),
                         sizeof(PTE))) {
          break;
     }
	if (PTE == 0) {
		break;
	}

	if (!VM_X64_PTE_IS_VALID(PTE)) {
		*phyAddress = PTE;
		break;
	}
	*phyAddress = (PTE & 0xFFFFFFFFFF000) + (virtualAddress & 0xFFF);
	bRet = TRUE;

  } while (FALSE);
  
  return bRet;

}
//
BOOLEAN VMReadVmVirtualAddr(PVOID TargetBuffer, DWORD64 DirectoryTableBase,
                    DWORD64 VirtualAddress, SIZE_T Size) {
	BOOLEAN bRet = FALSE;
	BOOLEAN bPhyAddr = 0;
	DWORD64 PhyAddr = 0;
	int AlignmentCount = 0;
	DWORD64 va = VirtualAddress;
	DWORD64 tb = (DWORD64)TargetBuffer;

	do
	{
         if (TargetBuffer == NULL || DirectoryTableBase == 0 ||
			 VirtualAddress == 0 || Size == 0) {
           break;
         }

	    AlignmentCount = ((Size + 0xFFF) & ~0xFFF) / 0x1000;

		for (int i = 0; i < AlignmentCount; i++)
		{
			bPhyAddr =
				VMTranslatePhyAddress(DirectoryTableBase, va, &PhyAddr);

			if (bPhyAddr || PhyAddr){

				//bPhyAddr为真，PhyAddr为有效PTE，直接读取,否则则是无效PTE，进入页处理去读取
				//得到虚拟地址的PTE，但这个PTE只决定当前粒度(4k,2m,1g)是有效的
		        //假如虚拟地址为：fffff804`4ae53400，size为0x1000，假设当前的PTE粒度为4K的情况
				//(目前建议最大粒度是0x1000,这样也可以涵盖2M和1G，可以减少判断代码
		        //那fffff804`4ae53000-fffff804`4ae53FFF是有效的，剩下的0x1000-0x400是跨页的，需要重新判断PTE
				bPhyAddr ? VMReadHostRegion((void*)(tb), PhyAddr, ADDRESS_CALC(va)) : 
					VMReadPaged((void*)(tb), DirectoryTableBase,va,PhyAddr, ADDRESS_CALC(va));
			}
			tb += ADDRESS_CALC(va);
			va += ADDRESS_CALC(va);
		
		}

		bRet = TRUE;

	} while (FALSE);
	
	return bRet;
}

BOOL VMWriteHostRegion(PVOID buffer, ULONG64 addr, SIZE_T size) {
  return WriteProcessMemory(gVmWareProcessHandle,
                            (void *)((ULONG64)gMemBaseInfo.BaseAddress + addr),
                            buffer, size, NULL);
}


BOOLEAN VMWriteVmVirtualAddr(PVOID sourceBuffer, DWORD64 directoryTableBase,
                     DWORD64 virtualAddress, SIZE_T size) {
	DebugBreak();
 /* DWORD64 physicalAddress =
      VMTranslatePhyAddress(directoryTableBase, virtualAddress);
  if (!physicalAddress) return FALSE;*/

//这里要修改
	directoryTableBase;
	virtualAddress;
	DWORD64 physicalAddress = 0;
  return (BOOLEAN)VMWriteHostRegion(sourceBuffer, physicalAddress, size);
}

VOID *VMGetExportsFunAddr(DWORD64 ModuleBaseAddr, CHAR* FunName, BOOLEAN IsFun) {
  void *ret = NULL;
  IMAGE_DOS_HEADER dosHeader = {0};
  IMAGE_NT_HEADERS64 ntHeaders = {0};
  IMAGE_DATA_DIRECTORY *dataDirectory = NULL;
  char *exportsBuffer = NULL;
  do {
    VMReadVmVirtualAddr(&dosHeader, gNtProcessData.MemoryKernelDirbase, ModuleBaseAddr,
                sizeof(IMAGE_DOS_HEADER));
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
      printf("%s DOS signature does not match", FunName);
      break;
    }

    VMReadVmVirtualAddr(&ntHeaders, gNtProcessData.MemoryKernelDirbase,
                ModuleBaseAddr + dosHeader.e_lfanew,
                sizeof(IMAGE_NT_HEADERS64));
    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
      printf("NT header signature does not match");
      break;
    }

    dataDirectory =
        ntHeaders.OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT;

    exportsBuffer = malloc(dataDirectory->Size + 1);
    memset(exportsBuffer, 0, dataDirectory->Size + 1);
    IMAGE_EXPORT_DIRECTORY *exportsDirectory =
        (IMAGE_EXPORT_DIRECTORY *)exportsBuffer;

    if (!VMReadVmVirtualAddr(exportsBuffer, gNtProcessData.MemoryKernelDirbase,
                     ModuleBaseAddr + dataDirectory->VirtualAddress,
                     dataDirectory->Size)) {
      printf("Failed to read exports directory");
      break;
    }
    exportsBuffer[dataDirectory->Size] = 0;
    if (!exportsDirectory->NumberOfNames || !exportsDirectory->AddressOfNames) {
      printf("Zero exports found");
      break;
    }

    DWORD exportOffset = dataDirectory->VirtualAddress;
    DWORD *names =
        (DWORD *)(void *)(exportsBuffer + exportsDirectory->AddressOfNames -
                          exportOffset);
    if (exportsDirectory->AddressOfNames - exportOffset +
            exportsDirectory->NumberOfNames * sizeof(DWORD) >
        dataDirectory->Size) {
      printf("Boundary check fail (1)");
      break;
    }

    USHORT *ordinals =
        (USHORT *)(void *)(exportsBuffer +
                           exportsDirectory->AddressOfNameOrdinals -
                           exportOffset);
    if (exportsDirectory->AddressOfNameOrdinals - exportOffset +
            exportsDirectory->NumberOfNames * sizeof(USHORT) >
        dataDirectory->Size) {
      printf("Boundary check fail (2)");
      break;
    }

    DWORD *functions =
        (DWORD *)(void *)(exportsBuffer + exportsDirectory->AddressOfFunctions -
                          exportOffset);
    if (exportsDirectory->AddressOfFunctions - exportOffset +
            exportsDirectory->NumberOfFunctions * sizeof(DWORD) >
        dataDirectory->Size) {
      printf("Boundary check fail (3)");
      break;
    }
    for (DWORD i = 0; i < exportsDirectory->NumberOfNames; i++) {
      if (names[i] > dataDirectory->Size + exportOffset ||
          names[i] < exportOffset ||
          ordinals[i] > exportsDirectory->NumberOfNames)
        continue;

      char *exportName = _strdup(exportsBuffer + names[i] - exportOffset);
      DWORD64 exportAddress = ModuleBaseAddr + functions[ordinals[i]];
      if (_stricmp(exportName, FunName) == 0) {
        if (IsFun) {
          ret = (void *)exportAddress;
        } else {
          VMReadVmVirtualAddr(&ret, gNtProcessData.MemoryKernelDirbase, exportAddress,
                      sizeof(DWORD64));
        }
		break;
      }
    }

  } while (FALSE);

  if (exportsBuffer) {
    free(exportsBuffer);
  }
  return ret;
}
//
BOOLEAN VMFindKernel() {
  char buffer[0x1000];

  for (DWORD64 i = (gNtProcessData.MemoryKernelEntry & ~0x1fffff) + 0x20000000;
       i > gNtProcessData.MemoryKernelEntry - 0x20000000; i -= 0x1000) {
    if (!VMReadVmVirtualAddr(buffer, gNtProcessData.MemoryKernelDirbase, i, 0x1000)) {
      continue;
    }
    if ((*(short *)(void *)(buffer) == IMAGE_DOS_SIGNATURE)) {
      int kdbg = 0, poolCode = 0;
      for (int u = 0; u < 0x1000; u++) {
        kdbg = kdbg ||
               *(DWORD64 *)(void *)(buffer + u) ==
                   0x4742444b54494e49;  // INITKDBG:
                                        // InitKDBG节保存了PG保护的主要检测的逻辑
        poolCode = poolCode || *(DWORD64 *)(void *)(buffer + u) ==
                                   0x45444f434c4f4f50;  // POOLCODE

        if (kdbg & poolCode) {
          printf("MemoryKernelBase:%p\n", (void *)i);
          gNtProcessData.MemoryKernelBase = i;
          gNtProcessData.SystemProcessEprocess = (DWORD64)VMGetExportsFunAddr(
              gNtProcessData.MemoryKernelBase, "PsInitialSystemProcess", FALSE);
		  gNtProcessData.PsLoadedModuleListPtr = VMGetExportsFunAddr(gNtProcessData.MemoryKernelBase, "PsLoadedModuleList", TRUE);
		  return (gNtProcessData.SystemProcessEprocess && gNtProcessData.PsLoadedModuleListPtr) ? TRUE : FALSE;
        }
      }
    }
  }
  return FALSE;
}
//
BOOLEAN VMMemoryInit(DWORD ProcessId) {
  BOOLEAN status = FALSE;
  do {
    printf("Opening the vmware process...\n");
    gVmWareProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
    if (NULL == gVmWareProcessHandle) {
      printf("Failed to open the vmware process.ErrorCode:%d\n",
             GetLastError());
      break;
    }
    printf("Start the memory region lookup.\n");
    if (!VMFindMappedRegion()) {
      printf("Mapped region not found.\n");
      break;
    }
    printf("Start scanning memory...\n");
    if (!VMNtKernelDataInit()) {
      printf("Failed to find Nt Kernel Data.\n");
      break;
    }
    printf("SystemCr3: 0x%p / UEFI KernelEntry : 0x%p.\n", (void*)gNtProcessData.MemoryKernelDirbase,
		(void*)gNtProcessData.MemoryKernelEntry);

    printf("Start looking for the NT kernel...\n");
    if (!VMFindKernel()) {
      printf("Failed to NT find kernel.\n");
      break;
    }

	status = TRUE;
  } while (FALSE); 

  return status;
}

BOOLEAN VMFindVmProcessData(CHAR *ProcessName, VM_PROCESS_DATA* VmProcessData) {
  BOOLEAN status = FALSE;
  DWORD64 SystemProcessActiveProcessLinksAddr = 0;
  LIST_ENTRY TempList = {0};
  LIST_ENTRY *p = NULL;
  CHAR ImageFileName[15] = {0};
  if (ProcessName == NULL) {
    return FALSE;
  }
  //得到system进程eprocess的ActiveProcessLinksAddr
  SystemProcessActiveProcessLinksAddr =
	  gNtProcessData.SystemProcessEprocess +
	  gNtProcessOffset.ActiveProcessLinksOffset;

  //遍历进程链表查找目标进程
  p = (LIST_ENTRY *)SystemProcessActiveProcessLinksAddr;
  do {
    //读取进程名
    if (!VMReadVmVirtualAddr(
            ImageFileName, gNtProcessData.MemoryKernelDirbase,
            ((DWORD64)p - gNtProcessOffset.ActiveProcessLinksOffset) +
		gNtProcessOffset.ImageFileNameOffset,
            sizeof(ImageFileName))) {
      break;
    }
    //如果相等，则找到进程EPROCESS地址,并得到需要的信息
    if (strcmp(ImageFileName, ProcessName) == 0) {
		VmProcessData->DestProcessEprocess =((DWORD64)p - gNtProcessOffset.ActiveProcessLinksOffset);
		//读取CR3
		if (!VMReadVmVirtualAddr(
			&VmProcessData->DestProcessCr3, gNtProcessData.MemoryKernelDirbase,
			((DWORD64)VmProcessData->DestProcessEprocess +
				gNtProcessOffset.DirectoryTableBaseOffset),
			sizeof(ULONGLONG))) {
			printf("Failed to get target CR3.\n");
			break;
		}
      status = TRUE;
      break;
    }

    memset(&TempList, 0, sizeof(LIST_ENTRY));
    if (!VMReadVmVirtualAddr(&TempList, gNtProcessData.MemoryKernelDirbase, (DWORD64)p, sizeof(LIST_ENTRY))) {
      break;
    }

    p = TempList.Blink;
  } while (p != (LIST_ENTRY *)SystemProcessActiveProcessLinksAddr);

  return status;
}

BOOLEAN VMNtKernelAddressOffsetInit()
{
	//目前只写了64位的
	return VMGetWinX64ProcessOffset(&gNtProcessData);
}

BOOLEAN VmWareThroughInit(DWORD VmWarePid)
{
	BOOLEAN status = FALSE;
	do
	{
		if (VmWarePid == 0)
		{
			printf("VmWareThroughInit: Parameter Error.\n");
			break;
		}		
		//内存初始化
		if (!VMMemoryInit(VmWarePid))
		{
			printf("VmWareThroughInit: Memory Initialization Failed.\n");
			break;
		}
		//Nt内核数据初始化
		if (!VMNtKernelAddressOffsetInit())
		{
			printf("VmWareThroughInit: Nt Kernel Address Offset Initialization Failed.\n");
			break;
		}
	
		status = TRUE;

	} while (FALSE);

	return status;

}

DWORD64 VMGetModuleBaseAddr(int ModuleID)
{
	DWORD64 BaseAddr = 0;
	//目前只实现了NT的
	switch (ModuleID)
	{
	case NT_KERNEL_ID:
		BaseAddr = gNtProcessData.MemoryKernelBase;
		break;
	default:
		break;
	}
	return BaseAddr;
}

BOOLEAN VMLoadDriver(CHAR* VmDriverLoadByImagePath)
{
	BOOLEAN bRet = FALSE;
	BOOLEAN bReadAddr = TRUE;
	KMJDATA pk = { 0 };
	do
	{
		if (NULL == VmDriverLoadByImagePath ||
			(strlen(VmDriverLoadByImagePath) >= MAX_PATH)) {
			break;
		}
		//读通信数据
		if (!KMJGetCommPhyAddr()) {
			break;
		}
		if (!VMReadHostRegion(&pk, KMJGetCommPhyAddr(), sizeof(KMJDATA))) {
			break;
		}
		//修改数据
		pk.op = KMJ_LOAD_DRIVER;
		strcpy_s(pk.DataInStr, MAX_PATH, VmDriverLoadByImagePath);

		if (!VMWriteHostRegion(&pk, KMJGetCommPhyAddr(), sizeof(KMJDATA))) {
			break;
		}
		//等待结果
		do {
			Sleep(100);
			if (!VMReadHostRegion(&pk, KMJGetCommPhyAddr(), sizeof(KMJDATA))) {
				bReadAddr = FALSE;
				break;
			}

		} while (pk.op != KMJ_COMPLETED);

		if (!bReadAddr || pk.DataOut[0] != TRUE) {
			break;
		}

		bRet = TRUE;

	} while (FALSE);

	return bRet;

}

BOOLEAN VMReadPaged(PVOID buffer, DWORD64 DirectoryTableBase, DWORD64 va, DWORD64 pte, SIZE_T size)
{
	BOOLEAN bRet = FALSE;
	DWORD64 ppa = 0;
	do
	{
		if (!buffer || !pte || !size || !va) {
			break;
		}

		if (!MemX64ReadPaged(va, pte, DirectoryTableBase, &ppa)) {
			break;
		}
		if (!VMReadHostRegion(buffer, (ppa | (va & 0xFFF)), size)){
			break;
		}

		bRet = TRUE;
	} while (FALSE);

	return bRet;

}

NT_PROCESS_DATA* VMGetNtKernelData()
{
	return &gNtProcessData;
}

MEMORY_BASIC_INFORMATION* VMGetHostMemBasicInfo()
{
	return &gMemBaseInfo;
}

HANDLE VMGetVmwareProcHandle()
{
	return gVmWareProcessHandle;
}

VM_PROCESS_DATA* VMGetVmwareDestProcData()
{
	return &gDestProcessData;
}

