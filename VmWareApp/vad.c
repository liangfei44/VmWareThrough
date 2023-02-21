#include "vad.h"
#include "VmWareApp.h"
#include <stdio.h>


BOOLEAN VADVerificationPoolTag(_In_ DWORD dwPoolTag, _In_ DWORD cPoolTag, ...);
// Win8.1/10 64-bit
typedef struct _MMVAD_X64W10 {
	DWORD _Dummy1;
	DWORD PoolTag;
	DWORD64 _Dummy2;
	// _MMVAD_SHORT
	DWORD64 LeftChild;
	DWORD64 RightChild;
	DWORD64 ParentValue;
	DWORD StartingVpn;
	DWORD EndingVpn;
	BYTE StartingVpnHigh;
	BYTE EndingVpnHigh;
	BYTE CommitChargeHigh;
	BYTE SpareNT64VadUChar;
	DWORD _Filler1;
	DWORD64 PushLock;
	DWORD u;    // 在Win10中，结构位的顺序变化不大
	union {
		struct {
			int CommitCharge : 31;   // Pos 0
			int MemCommit : 1;    // Pos 31
		}u1_flags;
		DWORD u1;
	}U1;
	DWORD64 EventList;
	// _MMVAD
	union {
		struct {
			int FileOffset : 24;   // Pos 0
			int Large : 1;    // Pos 24
			int TrimBehind : 1;    // Pos 25
			int Inherit : 1;    // Pos 26
			int CopyOnWrite : 1;    // Pos 27
			int NoValidationNeeded : 1;   // Pos 28
			int _Spare2 : 3;    // Pos 29
		}u2_flags;
		DWORD64 u2;
	}U2;
	DWORD64 Subsection;
	DWORD64 FirstPrototypePte;
	DWORD64 LastContiguousPte;
	DWORD64 ViewLinks[2];
	DWORD64 VadsProcess;
	DWORD64 u4;
	DWORD64 FileObject;
} MMVAD_X64W10,*PMMVAD_X64W10;


// 遍历VAD树,这里最好写一个回调处理，这样后续遍历VAD，可以处理不同的事件
// 这里懒得写了，直接判断
VOID VADTraverseVad(_In_ DWORD64 vad,_In_ DWORD64 VirAddr ,_Out_ PDWORD64 VadPte) {
  MMVAD_X64W10 MmVad = { 0 };
  DWORD64 StartAddr = 0;
  DWORD64 EndAddr = 0;
  if (vad) {
	  //读取VadRoot
	  if (!VMReadVmVirtualAddr(&MmVad,
		                        VMGetNtKernelData()->MemoryKernelDirbase,
		                        vad-0x10,
		                        sizeof(MMVAD_X64W10))) {
		  return;
	  }
	  //验证参数是否正确
	  if ((MmVad.EndingVpnHigh < MmVad.StartingVpnHigh) ||
		                        (MmVad.EndingVpn < MmVad.StartingVpn) ||
		                        !VADVerificationPoolTag(MmVad.PoolTag, 5, VAD_POOLTAG_VADS, VAD_POOLTAG_VAD, VAD_POOLTAG_VADL, VAD_POOLTAG_VADM, VAD_POOLTAG_VADF)) {
		  return;
	  }

	  //12位的偏移位这里要考虑到
	  StartAddr = ((DWORD64)MmVad.StartingVpnHigh << (32 + 12)) | ((DWORD64)MmVad.StartingVpn << 12);
	  EndAddr = ((DWORD64)MmVad.EndingVpnHigh << (32 + 12)) | ((DWORD64)MmVad.EndingVpn << 12) | 0xfff;
	  //目前不考虑压缩内存的算法，这里就简单的处理
	  if (VirAddr >= StartAddr && VirAddr <= EndAddr) {
		  //代表这里是MMVAD_SHORT,不存在Subsection/FirstPrototypePte/FileObject
		  if (MmVad.PoolTag == VAD_POOLTAG_VADS) {
			  return;
		  }
		  DebugBreak();
	  }

    if (MmVad.LeftChild) VADTraverseVad(MmVad.LeftChild, VirAddr, VadPte);
    if (MmVad.RightChild) VADTraverseVad(MmVad.RightChild, VirAddr, VadPte);
  }
}

VOID VADFindVadByTree(_In_ DWORD64 VadRoot,_In_ DWORD64 VirAddr,_Out_ PDWORD64 VadPte)
{
  if (!VirAddr || !VadRoot || !VadPte) {
    return;
  }

  VADTraverseVad(VadRoot, VirAddr, VadPte);
}


BOOLEAN VADVerificationPoolTag(_In_ DWORD dwPoolTag, _In_ DWORD cPoolTag, ...)
{
	va_list argp;
	dwPoolTag = _byteswap_ulong(dwPoolTag);
	va_start(argp, cPoolTag);
	while (cPoolTag) {
		if (dwPoolTag == va_arg(argp, DWORD)) {
			va_end(argp);
			return TRUE;
		}
		cPoolTag--;
	}
	va_end(argp);
	return FALSE;
}

DWORD64 VADFindVadPte(_In_ DWORD64 VirAddr)
{
	DWORD64 VadRoot = 0;
	DWORD64 VadCount = 0;
	DWORD64 VadPte = 0;
	do
	{		
		if (!VirAddr) {
			break;
		}
		//读取VadCount
		if (!VMReadVmVirtualAddr(&VadCount,
			                     VMGetNtKernelData()->MemoryKernelDirbase,
			                     VMGetVmwareDestProcData()->DestProcessEprocess +
			                     VMGetNtProcOffset()->VadRootOffset + 0x10,
			                     8)) {
			break;
		}

		if (!VadCount) {
			break;
		}
		//读取VadRoot
		if (!VMReadVmVirtualAddr(&VadRoot,
			                     VMGetNtKernelData()->MemoryKernelDirbase,
			                     VMGetVmwareDestProcData()->DestProcessEprocess +
			                                               VMGetNtProcOffset()->VadRootOffset,
			                     sizeof(DWORD64))) {
			break;
		}

		if (!VadRoot) {
			break;
		}  

		VADFindVadByTree(VadRoot, VirAddr, &VadPte);

	} while (FALSE);

	return VadPte;
}