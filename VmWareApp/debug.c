#include "debug.h"
#include "stdio.h"


BOOLEAN DebugProcess(DWORD pid)
{
	DEBUG_EVENT DebugEv;                   // debugging event information 
	DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation 
	BOOLEAN bRet = TRUE;

	__try
	{
		if (!DebugActiveProcess(pid))
		{
			printf("DebugActiveProcess Failed,ErrorCode:%d.\n", GetLastError());
			__leave;
		}
		for (;;)
		{
			WaitForDebugEvent(&DebugEv, INFINITE);

			// Process the debugging event code. 
			
			switch (DebugEv.dwDebugEventCode)
			{				
			case EXCEPTION_DEBUG_EVENT:
			{
				printf("ExceptionAddress=%p\n", DebugEv.u.Exception.ExceptionRecord.ExceptionAddress);
				switch (DebugEv.u.Exception.ExceptionRecord.ExceptionCode)
				{
				case EXCEPTION_BREAKPOINT:
				{
					break;
				} // case EXCEPTION_BREAKPOINT
				case EXCEPTION_ACCESS_VIOLATION:
				case EXCEPTION_GUARD_PAGE:
				case EXCEPTION_IN_PAGE_ERROR:
					printf("EXCEPTION_GUARD_PAGE.\n");
					break;

				default:
				{
					dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
					break;
				}
				} // switch (DebugEv.u.Exception.ExceptionRecord.ExceptionCode)

				break;
			} // case EXCEPTION_DEBUG_EVENT
			default:
				break;
			} // switch (DebugEv.dwDebugEventCode) 

			// Resume executing the thread that reported the debugging event. 

			if (ContinueDebugEvent(DebugEv.dwProcessId,
                                               DebugEv.dwThreadId,
                                               dwContinueStatus) == 0)
			{
				printf("ContinueDebugEvent Failed,ErrorCode:%d.\n", GetLastError());
				__leave;
			}
		}
	} // try
	__finally
	{
		if (DebugActiveProcessStop(pid) == FALSE)
		{
			printf("DebugActiveProcessStop Failed,ErrorCode:%d.\n", GetLastError());
		}
	} // __finally
	return bRet;

}
