//// pe.c : 与在虚拟地址空间中解析PE映像相关的实现。
//// 作 者: 学技术打豆豆
#include "pe.h"
#include "VmWareApp.h"
#include "misc.h"

PIMAGE_NT_HEADERS PEGetNtHeader(_In_ DWORD64 VmModuleBase, _Inout_ PBYTE VmModuleHeader, _Out_opt_ PBOOLEAN isX64)
{
	PIMAGE_DOS_HEADER DosHeader = NULL;
	PIMAGE_NT_HEADERS NtHeader = NULL;

	do
	{
		if (0 == VmModuleBase || NULL == VmModuleHeader){
			break;
		}

		if (isX64){
			*isX64 = FALSE;
		}
		
		if (!VMReadVmVirtualAddr(VmModuleHeader,
                                       VMGetNtKernelData()->MemoryKernelDirbase,
                                       VmModuleBase, PAGE_SIZE)){
			break;
		}

		DosHeader = (PIMAGE_DOS_HEADER)VmModuleHeader;
		if (!DosHeader || DosHeader->e_magic != IMAGE_DOS_SIGNATURE) { 
			break;
		}
		if ((DosHeader->e_lfanew < 0) || (DosHeader->e_lfanew > 0x800)) {
			break;
		}
		NtHeader = (PIMAGE_NT_HEADERS)(VmModuleHeader + DosHeader->e_lfanew);
		if (!NtHeader || NtHeader->Signature != IMAGE_NT_SIGNATURE) { 
			break;
		}
        if ((NtHeader->OptionalHeader.Magic !=IMAGE_NT_OPTIONAL_HDR64_MAGIC) &&
                    (NtHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)) {
			break;
		}
		if (isX64) { 
			*isX64 = (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
		}


	} while (FALSE);	
	
	return NtHeader;
}

DWORD PEGetSectionNumber(_In_ DWORD64 VmModuleBase)
{
	BOOLEAN isX64;
	BYTE VmModuleHeader[0x1000] = { 0 };
	DWORD SectionNumber = 0;
	PIMAGE_NT_HEADERS NtHeader = NULL;
	do
	{
		if (0 == VmModuleBase)
		{
			break;
		}
		NtHeader = PEGetNtHeader(VmModuleBase, VmModuleHeader, &isX64);
		if (!NtHeader)
		{
			break;
		}
        SectionNumber = isX64 ? ((PIMAGE_NT_HEADERS64)NtHeader)->FileHeader.NumberOfSections: 
					            ((PIMAGE_NT_HEADERS32)NtHeader)->FileHeader.NumberOfSections;
		if (SectionNumber > 0x40)
		{
			SectionNumber = 0;
			break;
		}

	} while (FALSE);

	return SectionNumber;
}

DWORD64 PEGetSectionsBaseAddr(_In_ DWORD64 VmModuleBase)
{
	BOOLEAN isX64;
	BYTE VmModuleHeader[0x1000] = { 0 };
	DWORD64 SectionsBaseAddr = 0;
	PIMAGE_NT_HEADERS NtHeader = NULL;
	do
	{
		if (0 == VmModuleBase)
		{
			break;
		}
		NtHeader = PEGetNtHeader(VmModuleBase, VmModuleHeader, &isX64);
		if (!NtHeader)
		{
			break;
		}
		SectionsBaseAddr = isX64 ?
			((DWORD64)NtHeader + sizeof(IMAGE_NT_HEADERS64)) :
			((DWORD64)NtHeader + sizeof(IMAGE_NT_HEADERS32));

		SectionsBaseAddr -= (DWORD64)VmModuleHeader;
		SectionsBaseAddr += VmModuleBase;

	} while (FALSE);

	return SectionsBaseAddr;
}

