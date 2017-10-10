#ifndef lucy_prefix_h
#define lucy_prefix_h

#undef l_cmpl_clang
#undef l_cmpl_icc
#undef l_cmpl_gcc
#undef l_cmpl_msc

#if defined(__clang__)
#define l_cmpl_clang /* Clang/LLVM, also defines __GNUC__ or __GNUG__ */
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#define l_cmpl_icc /* Intel ICC/ICPC, also defines __GNUC__ or __GNUG__ */
#elif defined(__GNUC__) || defined(__GNUG__)
#define l_cmpl_gcc /* GNU GCC/G++ */
#elif defined(_MSC_VER)
#define l_cmpl_msc /* Microsoft Visual Studio */
#endif

/** OS detection using compiler's predefined macros **
Compiler         C macros                       C++ macros
Clang/LLVM       clang -dM -E -x c /dev/null    clang++ -dM -E -x c++ /dev/null
GNU GCC/G++      gcc   -dM -E -x c /dev/null    g++     -dM -E -x c++ /dev/null
Intel ICC/ICPC   icc   -dM -E -x c /dev/null    icpc    -dM -E -x c++ /dev/null */

#undef l_plat_windows
#undef l_plat_linux
#undef l_plat_apple
#undef l_plat_osx
#undef l_plat_ios
#undef l_plat_bsd

#if defined(__linux__)
#define l_plat_linux /* Linux (Centos, Debian, Fedora, OpenSUSE, RedHat, Ubuntu) */
#elif defined(__APPLE__) && defined(__MACH__)
#define l_plat_apple /* Apple OSX and iOS (Darwin) */
#include <TargetConditionals.h>
/*                      Mac OSX  iOS	  iOS Simulator
TARGET_OS_EMBEDDED      0        1        0
TARGET_OS_IPHONE        0        1        1
TARGET_OS_MAC           1        1        1
TARGET_IPHONE_SIMULATOR 0        0        1 */
#if TARGET_OS_EMBEDDED == 1 || TARGET_OS_IPHONE == 1
#define l_plat_ios /* iPhone, iPad, simulator, etc. */
#elif TARGET_OS_MAC == 1
#define l_plat_osx /* MACOSX */
#endif
#elif defined(__unix__)
#include <sys/param.h>
#if defined(BSD)
#define l_plat_bsd /* DragonFly BSD, FreeBSD, OpenBSD, NetBSD */
#endif
#elif defined(_MSC_VER) && defined(_WIN32)
#define l_plat_windows /* Microsoft Windows */
#endif

#undef L_NEWLINE
#undef L_NL_SIZE
#undef L_PATH_SEP

#if defined(l_plat_windows)
#define L_NEWLINE "\r\n"
#define L_NL_SIZE 2
#define L_PATH_SEP "\\"
#else
#define L_NEWLINE "\n"
#define L_NL_SIZE 1
#define L_PATH_SEP "/"
#endif

#undef L_PLAT_IMPL_SIZE
#define L_PLAT_IMPL_SIZE(n) l_eightbyte impl[((n) - 1) / sizeof(l_eightbyte) + 1]

typedef union {
  double dval;
  char a[8];
  void* pval;
} l_eightbyte;

#endif /* lucy_prefix_h */
