#include "vad.h"
#include "VmWareApp.h"

VOID VADFindVadByPte()
{
	DWORD VadCount = 0;
	do
	{		
		//¶ÁÈ¡VadCount,
		if (!VMReadVmVirtualAddr(&VadCount,
			VMGetNtKernelData()->MemoryKernelDirbase,
			VMGetVmwareDestProcData()->DestProcessEprocess +
			VMGetNtProcOffset()->VadRootOffset + 0x10,
			8)) {
			break;
		}

	} while (FALSE);
}