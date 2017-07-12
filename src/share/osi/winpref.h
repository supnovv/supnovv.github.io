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
/**
 * WINVER: windows version - can used to enable features on specific versions
 * such as 0x0501 for Windows XP and 0x0600 for Windows Vista.
 * _WIN16/_WIN32/_WIN64: 16/32/64-bit windows platform.
 * _WIN32 is also defined on 64-bit platform for backward compatibility.
 * _M_IX86/_M_X64/_M_IA64: for x86/x64/(intel itanium) architecture.
 */

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
//WINVER – used to enable features only available in newer operating systems.
//Define it to 0x0501 for Windows XP, and 0x0600 for Windows Vista.
#include <windows.h>

// https://msdn.microsoft.com/en-us/library/aa384198(VS.85).aspx

#if defined(_WIN16)
#define PLATFORM_WIN16 1
#else
#define PLATFORM_WIN16 0
#endif

#if defined(_WIN32)
#define PLATFORM_WIN32 1
#else
#define PLATFORM_WIN32 0
#endif

/* _WIN32 is also defined by the 64-bit compiler for backward compatibility */
#if defined(_WIN64)
#define PLATFORM_WIN64 1
#else
#define PLATFORM_WIN64 0
#endif

#if defined(_M_IX86)
#define ARCHITECTURE_X86 1
#else
#define ARCHITECTURE_X86 0
#endif

#if defined(_M_X64)
#define ARCHITECTURE_X64 1
#else
#define ARCHITECTURE_X64 0
#endif

/* Intel Itanium platform */
#if defined(_M_IA64)
#define ARCHITECTURE_IA64 1
#else
#define ARCHITECTURE_IA64 0
#endif

#endif /* l_windows_lib_h */
