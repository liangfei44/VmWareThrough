#pragma once

#include <Windows.h>
#include "WindowsNt.h"

#define PAGE_SIZE 0x1000

#define UnicodeStringToWchar(US, pwsz) CopyMemory(pwsz, US->Buffer, US->Length); \
                                        pwsz[US->Length/sizeof(WCHAR)]=L'\0'



DWORD MiscGetCurrentVersion();
BOOLEAN UnicodeToChar(UNICODE_STRING_EX* UnicodeStr, LPSTR* CharStr);
int WcharToChar(LPCWSTR lpWideCharStr, LPSTR* lppMultiByteStr);
