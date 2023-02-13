#pragma once
#include <Windows.h>

#define KMJDATA_MAGIC       0x7788520778852088
#define KMJ_VOID			0xffff
#define KMJ_COMPLETED		0
#define KMJ_READ			1
#define KMJ_WRITE			2
#define KMJ_TERMINATE		3
#define KMJ_MEM_INFO		4
#define KMJ_EXEC		    5
#define KMJ_READ_VA			6
#define KMJ_WRITE_VA		7
#define KMJ_EXEC_EXTENDED	8
#define KMJ_LOAD_DRIVER  	9
#define KMJ_UNLOAD_DRIVER   10

#define OBJ_CASE_INSENSITIVE    				0x00000040
#define FILE_SYNCHRONOUS_IO_NONALERT			0x00000020
#define FILE_OPEN								0x00000001
#define OBJ_KERNEL_HANDLE       				0x00000200

#define STATUS_INVALID_PARAMETER         ((DWORD   )0xC000000DL)

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status) ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status) ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)



typedef enum _FUN
{
	FUN_ExFreePool,
	FUN_MmGetPhysicalAddress,
	FUN_PsCreateSystemThread,
	FUN_RtlCopyMemory,
	FUN_RtlZeroMemory,
	FUN_RtlInitAnsiString,
	FUN_RtlInitUnicodeString,
	FUN_RtlAnsiStringToUnicodeString,
	FUN_KeDelayExecutionThread,
	FUN_ZwLoadDriver,
	FUN_ZwUnloadDriver,
	FUN_ZwQuerySystemInformation,
	FUN_stricmp,
	FUN_ExAllocatePool,
	FUN_ZwClose,
	FUN_ZwCreateFile,
	FUN_RtlFreeUnicodeString,
	FUN_ZwCreateKey,
	FUN_ZwSetValueKey,
	FUN_wcscat,
	//测试
	FUN_DbgBreakPoint


}FUN;

typedef __int64	PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemProcessInformation = 5,
	SystemModuleInformation = 11,
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef enum _MODE {
	KernelMode,
	UserMode,
	MaximumMode
} MODE;

typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

typedef _IRQL_requires_same_ _Function_class_(KSTART_ROUTINE) VOID KSTART_ROUTINE(
	_In_ PVOID StartContext
);
typedef KSTART_ROUTINE *PKSTART_ROUTINE;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	_Field_size_bytes_part_(MaximumLength, Length) PWCH   Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;


typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;
typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

typedef enum _MEMORY_CACHING_TYPE {
	MmNonCached = 0,
	MmCached = 1,
	MmWriteCombined = 2,
	MmHardwareCoherentCached = 3,
	MmNonCachedUnordered = 4,
	MmUSWCCached = 5,
	MmMaximumCacheType = 6
} MEMORY_CACHING_TYPE;

typedef struct _PHYSICAL_MEMORY_RANGE {
	PHYSICAL_ADDRESS BaseAddress;
	LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY {
	ULONG Unknown1;
	ULONG Unknown2;
	ULONG Unknown3;
	ULONG Unknown4;
	PVOID Base;
	ULONG Size;
	ULONG Flags;
	USHORT Index;
	USHORT NameLength;
	USHORT LoadCount;
	USHORT PathLength;
	CHAR ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG Count;
	ULONG Unknown1;
	SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef struct _ANSI_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PCHAR  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID Pointer;
	}u1;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

typedef struct _NTOS {

	VOID(*FUN_ExFreePool)(
		_In_ PVOID P
		);
	PHYSICAL_ADDRESS(*FUN_MmGetPhysicalAddress)(
		_In_ PVOID BaseAddress
		);
	NTSTATUS(*FUN_PsCreateSystemThread)(
		_Out_      PHANDLE            ThreadHandle,
		_In_       ULONG              DesiredAccess,
		_In_opt_   POBJECT_ATTRIBUTES ObjectAttributes,
		_In_opt_   HANDLE             ProcessHandle,
		_Out_opt_  PCLIENT_ID         ClientId,
		_In_       PKSTART_ROUTINE    StartRoutine,
		_In_opt_   PVOID              StartContext
		);
	VOID(*FUN_RtlCopyMemory)(
		_Out_       VOID UNALIGNED *Destination,
		_In_  const VOID UNALIGNED *Source,
		_In_        SIZE_T         Length
		);
	VOID(*FUN_RtlZeroMemory)(
		_Out_ VOID UNALIGNED *Destination,
		_In_ SIZE_T Length
		);
	VOID(*FUN_RtlInitAnsiString)(
		_Out_    PANSI_STRING DestinationString,
		_In_opt_ PCSTR         SourceString
		);
	VOID(*FUN_RtlInitUnicodeString)(
		_Out_ PUNICODE_STRING DestinationString,
		_In_opt_ PCWSTR SourceString
		);
	NTSTATUS(*FUN_RtlAnsiStringToUnicodeString)(
		_Inout_ PUNICODE_STRING DestinationString,
		_In_    PANSI_STRING    SourceString,
		_In_    BOOLEAN         AllocateDestinationString
		);
	NTSTATUS(*FUN_KeDelayExecutionThread)(
		_In_ MODE            WaitMode,
		_In_ BOOLEAN         Alertable,
		_In_ PINT64          pllInterval_Neg100ns
		);
	NTSTATUS(*FUN_ZwLoadDriver)(
		_In_ PUNICODE_STRING DriverServiceName
		);
	NTSTATUS(*FUN_ZwUnloadDriver)(
		_In_ PUNICODE_STRING DriverServiceName
		);
	NTSTATUS(*FUN_ZwQuerySystemInformation)(
		_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
		_Inout_ PVOID SystemInformation,
		_In_ ULONG SystemInformationLength,
		_Out_opt_ PULONG ReturnLength);

	int(*FUN_stricmp)(
		const char *string1,
		const char *string2);

	PVOID(*FUN_ExAllocatePool)(
		_In_ DWORD64 PoolType,
		_In_ SIZE_T NumberOfBytes);

	NTSTATUS(*FUN_ZwClose)(
		_In_ HANDLE hObject
		);

	NTSTATUS(*FUN_ZwCreateFile)(
		_Out_	 PHANDLE			FileHandle,
		_In_	 ACCESS_MASK		DesiredAccess,
		_In_	 PVOID				ObjectAttributes,
		_Out_	 PIO_STATUS_BLOCK	IoStatusBlock,
		_In_opt_ PLARGE_INTEGER		AllocationSize,
		_In_	 ULONG				FileAttributes,
		_In_	 ULONG				ShareAccess,
		_In_	 ULONG				CreateDisposition,
		_In_	 ULONG				CreateOptions,
		_In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
		_In_	 ULONG				EaLength
		);

	VOID(*FUN_RtlFreeUnicodeString)(
		_Inout_ PUNICODE_STRING UnicodeString
		);

	NTSTATUS(*FUN_ZwCreateKey)(
		_Out_      PHANDLE            KeyHandle,
		_In_       ACCESS_MASK        DesiredAccess,
		_In_       POBJECT_ATTRIBUTES ObjectAttributes,
		_Reserved_ ULONG              TitleIndex,
		_In_opt_   PUNICODE_STRING    Class,
		_In_       ULONG              CreateOptions,
		_Out_opt_  PULONG             Disposition
		);

	NTSTATUS(*FUN_ZwSetValueKey)(
		_In_     HANDLE          KeyHandle,
		_In_     PUNICODE_STRING ValueName,
		_In_opt_ ULONG           TitleIndex,
		_In_     ULONG           Type,
		_In_opt_ PVOID           Data,
		_In_     ULONG           DataSize
		);

	wchar_t*(*FUN_wcscat)(
		wchar_t *strDestination,
		const wchar_t *strSource
		);

	//测试
	VOID (*FUN_DbgBreakPoint)();
	

} NTOS, *PNTOS;

//通信格式,目前只设计了这几个(宽度：0x1000大小)
typedef struct _KMJDATA {
	DWORD64 MAGIC;		     			
	DWORD64 KernelBaseAddr;
	DWORD64 OperatingSystem;			// 操作系统类型
	NTOS fn;                            // 内核函数地址
	DWORD64 op;                         // 操作码
	DWORD64 opStatus;                   // 操作状态
	DWORD64 Result;					    // 返回结果 TRUE|FALSE
	DWORD64 RWAddress;					// 读写地址
	DWORD64 RWSize;					    // 读写大小
	DWORD64 DMASizeBuffer;              // 客户端分配的内存大小
	DWORD64 DMAAddrPhysical;		    // 客户端内存的物理地址
	DWORD64 DMAAddrVirtual;		        // 客户端内存的虚拟地址
	CHAR DataInStr[MAX_PATH];	 	    // 通信数据（如：文件名等）  
	CHAR DataOutStr[MAX_PATH];          // 返回数据（如：文件名等）
	DWORD64 DataOut[28];				// 返回具体结果(状态码等)
 				
} KMJDATA, *PKMJDATA;

DWORD64 WX64STAGE3GetProcAddress(_In_ DWORD64 VmModuleBase, _In_ CHAR* ProcName);