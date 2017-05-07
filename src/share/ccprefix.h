#ifndef LIBC_CCPREFIX_H_
#define LIBC_CCPREFIX_H_

/** Compiler detection **/

#undef CC_CMPL_CLANG
#undef CC_CMPL_GCC
#undef CC_CMPL_MSC
#undef CC_CMPL_ICC

#if defined(__clang__)
#define CC_CMPL_CLANG /* Clang/LLVM, also defines __GNUC__ or __GNUG__ */
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#define CC_CMPL_ICC /* Intel ICC/ICPC, also defines __GNUC__ or __GNUG__ */
#elif defined(__GNUC__) || defined(__GNUG__)
#define CC_CMPL_GCC /* GNU GCC/G++ */
#elif defined(_MSC_VER)
#define CC_CMPL_MSC /* Microsoft Visual Studio */
#endif

/** OS detection using compiler's predefined macros **
Compiler         C macros                       C++ macros
Clang/LLVM       clang -dM -E -x c /dev/null    clang++ -dM -E -x c++ /dev/null
GNU GCC/G++      gcc   -dM -E -x c /dev/null    g++     -dM -E -x c++ /dev/null
Intel ICC/ICPC   icc   -dM -E -x c /dev/null    icpc    -dM -E -x c++ /dev/null */

#undef CC_OS_LINUX
#undef CC_OS_APPLE
#undef CC_APPLE_OSX
#undef CC_APPLE_IOS
#undef CC_OS_BSD
#undef CC_OS_WINDOWS
#undef CC_OS_WIN32
#undef CC_OS_WIN64

#if defined(__linux__)
#define CC_OS_LINUX /* Linux (Centos, Debian, Fedora, OpenSUSE, RedHat, Ubuntu) */
#elif defined(__APPLE__) && defined(__MACH__)
#define CC_OS_APPLE /* Apple OSX and iOS (Darwin) */
#include <TargetConditionals.h>
/*                      Mac OSX  iOS	  iOS Simulator
TARGET_OS_EMBEDDED      0        1        0
TARGET_OS_IPHONE        0        1        1
TARGET_OS_MAC           1        1        1
TARGET_IPHONE_SIMULATOR 0        0        1 */
#if TARGET_OS_EMBEDDED == 1 || TARGET_OS_IPHONE == 1
#define CC_APPLE_IOS /* iPhone, iPad, simulator, etc. */
#elif TARGET_OS_MAC == 1
#define CC_APPLE_OSX /* MACOSX */
#endif
#elif defined(__unix__)
#include <sys/param.h>
#if defined(BSD)
#define CC_OS_BSD /* DragonFly BSD, FreeBSD, OpenBSD, NetBSD */
#endif
#elif defined(_MSC_VER) && defined(_WIN32)
#define CC_OS_WINDOWS /* Microsoft Windows */
#if defined(_WIN64)
#define CC_OS_WIN64 /* 64-bit Windows */
#else
#define CC_OS_WIN32 /* 32-bit Windows */
#endif
#endif

#if defined(CC_OS_WINDOWS)
#define CCNEWLINE "\r\n"
#define CCNLSIZE 2
#else
#define CCNEWLINE "\n"
#define CCNLSIZE 1
#endif

union cceight {
  double dval;
  char a[8];
  void* pval;
};

#define CCPLAT_IMPL_SIZE(n) union cceight platimpl[((n) - 1) / sizeof(union cceight) + 1]

#endif /* LIBC_CCPREFIX_H_ */
