#ifndef l_windows_lib_h
#define l_windows_lib_h
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#if !defined(_WIN32) && !defined(_WIN64)
#error "_WIN32/_WIN64 should be defined"
#endif
/** Windows Version **
WINVER: windows version - can used to enable features on specific versions
such as 0x0501 for Windows XP and 0x0600 for Windows Vista.
_WIN16/_WIN32/_WIN64: 16/32/64-bit windows platform.
_WIN32 is also defined on 64-bit platform for backward compatibility.
_M_IX86/_M_X64/_M_IA64: for x86/x64/(intel itanium) architecture.
 ** Windows DLL **
#if defined(L_BUILD_SHARED)
#if defined(L_CORELIB_IMPL) || defined(L_WINDOWS_IMPL)
#define l_extern __declspec(dllexport)
#else
#define l_extern __declspec(dllimport)
#endif
#else
#define l_extern extern
#endif */
#endif /* l_windows_lib_h */

