#ifndef l_core_lib_h
#define l_core_lib_h
#include "l_prefix.h"
#include "l_auto.h"

#undef l_extern
#undef l_inline
#define l_inline static
#if defined(L_BUILD_SHARED)
  #if defined(__GNUC__)
  #define l_extern extern
  #else
  #if defined(L_CORELIB_IMPL) || defined(L_WINDOWS_IMPL)
  #define l_extern __declspec(dllexport)
  #else
  #define l_extern __declspec(dllimport)
  #endif
  #endif
#else
  #define l_extern extern
#endif

#define l_status_contread 3
#define l_status_waitmore 2
#define l_status_yield 1
#define l_status_ok 0
#define l_status_luaerr -1
#define l_status_error -2
#define l_status_eread -3
#define l_status_ewrite -4
#define l_status_elimit -5
#define l_status_ematch -6

#define l_cast(type, a) ((type)(a))
#define l_max_rwop_size (0x7fff0000) /* 2147418112 */
#define l_max_ubyte     l_cast(l_byte, 0xff) /* 255 */
#define l_max_ushort    l_cast(l_ushort, 0xffff) /* 65535 */
#define l_max_umedit    l_cast(l_umedit, 0xffffffff) /* 4294967295 */
#define l_max_uinteger  l_cast(l_uinteger, 0xffffffffffffffff) /* 18446744073709551615 */
#define l_max_sbyte     l_cast(l_sbyte, 0x7f) /* 127 */
#define l_max_short     l_cast(l_short, 0x7fff) /* 32767 */
#define l_max_medit     l_cast(l_medit, 0x7fffffff) /* 2147483647 */
#define l_max_integer   l_cast(l_integer, 0x7fffffffffffffff) /* 9223372036854775807 */
#define l_min_sbyte     l_cast(l_sbyte, -127-1) /* 128 0x80 */
#define l_min_short     l_cast(l_short, -32767-1) /* 32768 0x8000 */
#define l_min_medit     l_cast(l_medit, -2147483647-1) /* 2147483648 0x80000000 */
#define l_min_integer   l_cast(l_integer, -9223372036854775807-1) /* 9223372036854775808 0x8000000000000000 */

#ifdef L_BUILD_DEBUG
#define L_DEBUG_HERE(...) { __VA_ARGS__ }
#else
#define L_DEBUG_HERE(...) { ((void)0); }
#endif

#define L_MKSTR(a) #a
#define L_X_MKSTR(a) L_MKSTR(a)
#define L_MKFLSTR __FILE__ " (" L_X_MKSTR(__LINE__) ") "
#define L_STR(s) l_cast(l_rune*, s)

#define l_assert(e) l_assert_func((e), (#e), L_MKFLSTR)                           /* 0:assert */
#define l_log_e(fmt, ...) l_logger_func("1[E] " L_MKFLSTR, (fmt), ## __VA_ARGS__) /* 1:error */
#define l_log_w(fmt, ...) l_logger_func("2[W] " L_MKFLSTR, (fmt), ## __VA_ARGS__) /* 2:warning */
#define l_log_i(fmt, ...) l_logger_func("3[I] " L_MKFLSTR, (fmt), ## __VA_ARGS__) /* 3:important */
#define l_log_d(fmt, ...) l_logger_func("4[D] " L_MKFLSTR, (fmt), ## __VA_ARGS__) /* 4:debug */

l_extern void l_assert_func(int pass, const char* expr, const char* fileline);
l_extern int l_logger_func(const char* tag, const void* fmt, ...);
l_extern void l_set_log_level(int level);
l_extern int l_get_log_level();
l_extern void l_exit();

l_extern void* l_rawalloc(l_integer size);
l_extern void* l_rawalloc_init(l_integer size);
l_extern void* l_rawrelloc(void* buffer, l_integer oldsize, l_integer newsize);
l_extern void l_rawfree(void* buffer);

l_extern l_integer l_zerop(void* start, const void* beyond);
l_extern l_integer l_zerol(void* start, l_integer len);

/**
 * The 64-bit signed integer's biggest value is 9223372036854775807.
 * For seconds/milliseconds/microseconds/nanoseconds, it can
 * represent more than 291672107014/291672107/291672/291 years.
 * The 32-bit signed integer's biggest value is 2147483647.
 * For seconds/milliseconds/microseconds/nanoseconds, it can
 * represent more than 67-year/24-day/35-min/2-sec.
 */

#define l_nsecs_per_second (1000000000L)

typedef struct {
  l_integer sec;
  l_umedit nsec;
} l_time;

typedef struct {
  l_umedit_int yearlow; /* 38-bit, can represent 274877906943 years */
  l_umedit nsec;    /* nanoseconds that less than 1 sec */
  l_byte high;      /* high 6-bit are extra bits for year */
  l_byte ydaylow;   /* 1~366 */
  l_byte month;     /* 1~12 */
  l_byte day;       /* 1~31 */
  l_byte wday;      /* 0~6, 0 is sunday */
  l_byte hour;      /* 0~23 */
  l_byte min;       /* 0~59 */
  l_byte sec;       /* 0~61, 60 and 61 are the leap seconds */
} l_date;

typedef struct {
  l_integer fsize;
  l_integer ctime;
  l_integer atime;
  l_integer mtime;
  l_integer gid;
  l_integer uid;
  l_integer mode;
  l_byte isfile;
  l_byte isdir;
  l_byte islink;
} l_fileattr; /* creation, last access, last modify (UTC) */

typedef struct {
  void* stream;
} l_dir_stream;

l_extern l_time l_system_time();
l_extern l_time l_monotonic_time();
l_extern l_date l_system_date();
l_extern l_integer l_file_size(const void* name, int namelen);
l_extern l_fileattr l_file_attr(const void* name, int namelen);

l_extern void l_core_test();
l_extern void l_plat_test();

#endif /* l_core_lib_h */
