#ifndef lucy_prefix_h
#define lucy_prefix_h

/** Compiler detection **/

#undef L_CLER_CLANG
#undef L_CLER_GCC
#undef L_CLER_MSC
#undef L_CLER_ICC

#if defined(__clang__)
#define L_CLER_CLANG /* Clang/LLVM, also defines __GNUC__ or __GNUG__ */
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#define L_CLER_ICC /* Intel ICC/ICPC, also defines __GNUC__ or __GNUG__ */
#elif defined(__GNUC__) || defined(__GNUG__)
#define L_CLER_GCC /* GNU GCC/G++ */
#elif defined(_MSC_VER)
#define L_CLER_MSC /* Microsoft Visual Studio */
#endif

/** OS detection using compiler's predefined macros **
Compiler         C macros                       C++ macros
Clang/LLVM       clang -dM -E -x c /dev/null    clang++ -dM -E -x c++ /dev/null
GNU GCC/G++      gcc   -dM -E -x c /dev/null    g++     -dM -E -x c++ /dev/null
Intel ICC/ICPC   icc   -dM -E -x c /dev/null    icpc    -dM -E -x c++ /dev/null */

#undef L_PLAT_LINUX
#undef L_PLAT_APPLE
#undef L_PLAT_OSX
#undef L_PLAT_IOS
#undef L_PLAT_BSD
#undef L_PLAT_WINDOWS
#undef L_PLAT_WIN32
#undef L_PLAT_WIN64

#if defined(__linux__)
#define L_PLAT_LINUX /* Linux (Centos, Debian, Fedora, OpenSUSE, RedHat, Ubuntu) */
#elif defined(__APPLE__) && defined(__MACH__)
#define L_PLAT_APPLE /* Apple OSX and iOS (Darwin) */
#include <TargetConditionals.h>
/*                      Mac OSX  iOS	  iOS Simulator
TARGET_OS_EMBEDDED      0        1        0
TARGET_OS_IPHONE        0        1        1
TARGET_OS_MAC           1        1        1
TARGET_IPHONE_SIMULATOR 0        0        1 */
#if TARGET_OS_EMBEDDED == 1 || TARGET_OS_IPHONE == 1
#define L_PLAT_IOS /* iPhone, iPad, simulator, etc. */
#elif TARGET_OS_MAC == 1
#define L_PLAT_OSX /* MACOSX */
#endif
#elif defined(__unix__)
#include <sys/param.h>
#if defined(BSD)
#define L_PLAT_BSD /* DragonFly BSD, FreeBSD, OpenBSD, NetBSD */
#endif
#elif defined(_MSC_VER) && defined(_WIN32)
#define L_PLAT_WINDOWS /* Microsoft Windows */
#if defined(_WIN64)
#define L_PLAT_WIN64 /* 64-bit Windows */
#else
#define L_PLAT_WIN32 /* 32-bit Windows */
#endif
#endif

#if defined(L_PLAT_WINDOWS)
#define L_NEWLINE "\r\n"
#define L_NL_SIZE 2
#define L_PATH_SEP "\\"
#else
#define L_NEWLINE "\n"
#define L_NL_SIZE 1
#define L_PATH_SEP "/"
#endif

#define L_PLAT_IMPL_SIZE(n) l_eight impl[((n) - 1) / sizeof(l_eight) + 1]

typedef union {
  double dval;
  char a[8];
  void* pval;
} l_eight;

#endif /* lucy_prefix_h */
