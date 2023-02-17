#include "VmWareApp.h"
#include "KernelModeInject.h"
#include "debug.h"
#include "pe.h"
#include "VmWareDisk.h"

DWORD WINAPI ThreadProc(LPVOID lpThreadParameter) {
	DWORD pid = *(DWORD*)lpThreadParameter;

	DebugProcess(pid);
	return 0;
}

int main() {

	DWORD pid = 86268;
	if (!VmWareThroughInit(pid))
	{
		printf("main：VmWare Through Initialization Failed.\n");
		return -1;
	}
	//VmWare里需要操作的进程数据初始化,暂时不需要
	if (!VMFindVmProcessData("notepad.exe", VMGetVmwareDestProcData()))
	{
		printf("main: Find Process Data Failed.\n");
		return -1;
	}
	DWORD64 buf_addr = 0;
	for (int i = 0; i < 0x7FFFFFFFFFFF; i+=0x1000) {
		VMReadVmVirtualAddr(&buf_addr, VMGetVmwareDestProcData()->DestProcessCr3, i, 8);
	}

	//if (!KMIKernelInject())
	//{
	//	printf("main：Kernel injection failed.\n");
	//	return -1;
	//}
	//VMLoadDriver("\\??\\c:\\test\\DriverTest.sys");
	return 0;
}
