#pragma once
#include <Windows.h>

typedef struct _VM_LDR_MODULE_INFO
{
	DWORD64 BaseAddress;
	DWORD64 EntryPoint;

}VM_LDR_MODULE_INFO,*PVM_LDR_MODULE_INFO;

BOOLEAN KMIKernelInject();
DWORD KMJGetCommPhyAddr();
