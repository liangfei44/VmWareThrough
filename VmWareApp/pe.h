#pragma once

// pe.h : 在虚拟地址空间中解析PE映像的相关定义。
// 作 者: 学技术打豆豆
//
#include "windows.h"
#define PE_MAX_SUPPORTED_SIZE           0x20000000

DWORD PEGetSectionNumber(_In_ DWORD64 VmModuleBase);
DWORD64 PEGetSectionsBaseAddr(_In_ DWORD64 VmModuleBase);
