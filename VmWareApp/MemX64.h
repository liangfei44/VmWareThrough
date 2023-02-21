#pragma once
#include <windows.h>

#define MEM_IS_KERNEL_ADDR_X64(va)  (((va) & 0xffff800000000000) == 0xffff800000000000)

typedef enum VMM_PTE_TP {
	VMM_PTE_TP_NA = 0,
	VMM_PTE_TP_HARDWARE = 1,
	VMM_PTE_TP_TRANSITION = 2,
	VMM_PTE_TP_PROTOTYPE = 3,
	VMM_PTE_TP_DEMANDZERO = 4,
	VMM_PTE_TP_COMPRESSED = 5,
	VMM_PTE_TP_PAGEFILE = 6,
	VMM_PTE_TP_FILE_BACKED
} VMM_PTE_TP, *PVMM_PTE_TP;

#define VM_FLAG_NOVAD  0x00000001  // ²»Ç¶Ì×VADÁË

VMM_PTE_TP MemX64TransitionPaged(_In_ DWORD64 va, _In_ DWORD64 pte, _In_ DWORD64 DirectoryTableBase, _In_ DWORD64 Flags, _Out_ PDWORD64 ppa);
