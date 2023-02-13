#pragma once

#include <windows.h>

//X64和X32位下的EPROCESS的最大值(评估值,不是绝对准备，只要保证相应的字段在该值内就行)
#define VM_EPROCESS64_MAX_SIZE 0x800
#define VM_EPROCESS32_MAX_SIZE 0x480

typedef struct _UNICODE_STRING_EX {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING_EX, *PUNICODE_STRING_EX;

// _LDR_DATA_TABLE_ENTRY struct.
typedef struct _LDR_MODULE64 {
	LIST_ENTRY64        InLoadOrderModuleList;
	LIST_ENTRY64        InMemoryOrderModuleList;
	LIST_ENTRY64        InInitializationOrderModuleList;
	DWORD64               BaseAddress;
	DWORD64               EntryPoint;
	ULONG               SizeOfImage;
	ULONG               _Filler1;
	UNICODE_STRING_EX    FullDllName;
	UNICODE_STRING_EX    BaseDllName;
	ULONG               Flags;
	SHORT               LoadCount;
	SHORT               TlsIndex;
	LIST_ENTRY64        HashTableEntry;
	ULONG               TimeDateStamp;
	ULONG               _Filler2;
} LDR_MODULE64, *PLDR_MODULE64;

