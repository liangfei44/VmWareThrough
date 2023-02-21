#pragma once
#include <Windows.h>

//'Vadl' : '_MMVAD_LONG',
//'VadS' : '_MMVAD_SHORT',
//'Vad ' : '_MMVAD_LONG',
//'VadF' : '_MMVAD_SHORT',
//'Vadm' : '_MMVAD_LONG',
			

#define VAD_POOLTAG_VAD       'Vad '
#define VAD_POOLTAG_VADF      'VadF'
#define VAD_POOLTAG_VADS      'VadS'
#define VAD_POOLTAG_VADL      'Vadl'
#define VAD_POOLTAG_VADM      'Vadm'

DWORD64 VADFindVadPte(_In_ DWORD64 VirAddr);