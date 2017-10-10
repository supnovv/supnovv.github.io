#ifndef lucy_core_base_h
#define lucy_core_base_h
#include "autoconf.h"
#include "core/prefix.h"

#undef L_EXTERN
#undef L_PRIVAT
#undef L_GLOBAL
#undef L_INLINE
#undef L_THREAD_LOCAL
#undef L_THREAD_LOCAL_DECL
#undef L_THREAD_LOCAL_SUPPORTED

#if defined(L_BUILD_SHARED)
  #if defined(__GNUC__)
    #define L_EXTERN extern
  #else
    #if defined(L_LIBRARY_IMPL)
      #define L_EXTERN __declspec(dllexport)
    #else
      #define L_EXTERN __declspec(dllimport)
    #endif
  #endif
#else
  #define L_EXTERN extern
#endif

#define L_PRIVAT L_EXTERN
#define L_GLOBAL static
#define L_INLINE static

#if defined(l_cmpl_gcc)
  #define L_THREAD_LOCAL(...) __thread __VA_ARGS__
  #define L_THREAD_LOCAL_DECL(...) extern __thread __VA_ARGS__
  #define L_THREAD_LOCAL_SUPPORTED
#elif defined(l_cmpl_msc)
  #define L_THREAD_LOCAL(...) __declspec(thread) __VA_ARGS__
  #define L_THREAD_LOCAL_DECL(...) extern __declspec(thread) __VA_ARGS__
  #define L_THREAD_LOCAL_SUPPORTED
#else
  #define L_THREAD_LOCAL(...)
  #define L_THREAD_LOCAL_DECL(...)
#endif

#define l_cast(type, a) ((type)(a))
#define l_cstr(s) ((l_byte*)(s))

#define L_MAX_RWSIZE (0x7fff0000) /* 2147418112 */
#define L_MAX_UBYTE  ((l_byte)0xff) /* 255 */
#define L_MAX_SBYTE  ((l_sbyte)0x7f) /* 127 */
#define L_MIN_SBYTE  ((l_sbyte)-127-1) /* 128 0x80 */
#define L_MAX_USHORT ((l_ushort)0xffff) /* 65535 */
#define L_MAX_SHORT  ((l_short)0x7fff) /* 32767 */
#define L_MIN_SHORT  ((l_short)-32767-1) /* 32768 0x8000 */
#define L_MAX_UMEDIT ((l_umedit)0xffffffff) /* 4294967295 */
#define L_MAX_MEDIT  ((l_medit)0x7fffffff) /* 2147483647 */
#define L_MIN_MEDIT  ((l_medit)-2147483647-1) /* 2147483648 0x80000000 */
#define L_MAX_ULONG  ((l_ulong)0xffffffffffffffff) /* 18446744073709551615 */
#define L_MAX_LONG   ((l_long)0x7fffffffffffffff) /* 9223372036854775807 */
#define L_MIN_LONG   ((l_long)-9223372036854775807-1) /* 9223372036854775808 0x8000000000000000 */

#define L_SUCCESS  (0)
#define L_LUAYIELD (1)
#define L_WAITMORE (2)
#define L_CONTREAD (3)

#define L_ERROR  (-1)
#define L_ELUA   (-2)
#define L_EREAD  (-3)
#define L_EWRITE (-4)
#define L_ELIMIT (-5)
#define L_EMATCH (-6)
#define L_EINVAL (-7)

/**
 * memory operations
 */

#define l_mallocEx(allocfunc, ud, size) allocfunc((ud), 0, 0, (size))
#define l_callocEx(allocfunc, ud, size) allocfunc((ud), 0, 1, (size))
#define l_rallocEx(allocfunc, ud, buffer, oldsize, newsize) allocfunc((ud), (buffer), (oldsize), (newsize))
#define l_mfreeEx(allocfunc, ud, buffer) allocfunc((ud), (buffer), 0, 0)

#define l_malloc(allocfunc, size) l_mallocEx(allocfunc, 0, (size))
#define l_calloc(allocfunc, size) l_callocEx(allocfunc, 0, (size))
#define l_ralloc(allocfunc, buffer, oldsize, newsize) l_rallocEx(allocfunc, 0, (buffer), (oldsize), (newsize))
#define l_mfree(allocfunc, buffer) l_mfreeEx(allocfunc, 0, (buffer))

#define l_raw_malloc(size) l_malloc(l_raw_alloc_func, size)
#define l_raw_calloc(size) l_calloc(l_raw_alloc_func, size)
#define l_raw_ralloc(buffer, oldsize, newsize) l_ralloc(l_raw_alloc_func, buffer, oldsize, newsize)
#define l_raw_mfree(buffer) l_mfree(l_raw_alloc_func, buffer)

typedef void* (*l_allocfunc)(void* userdata, void* buffer, l_int oldsize, l_int newsize);
L_EXTERN void* l_raw_alloc_func(void* userdata, void* buffer, l_int oldsize, l_int newsize);

/**
 * basic string
 */

typedef struct {
  const l_byte* start;
  const l_byte* end;
} l_strt;

typedef struct {
  const l_byte* start;
  l_int len;
} l_strn;

#define l_strt_literal(s) l_strt_n(l_cstr("" s), sizeof(s) / sizeof(char) - 1)
#define l_strt_empty() ((l_strt){0,0})
#define l_strt_c(s) l_strt_n((s), strlen((char*)(s)))

#define l_strn_literal(s) ((l_strn){l_cstr("" s), sizeof(s) / sizeof(char) - 1})
#define l_strn_empty() ((l_strn){0,0})
#define l_strn_c(s) l_strn_n((s), strlen((char*)(s)))

L_INLINE l_strt
l_strt_from(const void* s, const void* e)
{
  return (l_strt){l_cstr(s), l_cstr(e)};
}

L_INLINE l_strt
l_strt_n(const void* s, l_int len)
{
  return (l_strt){l_cstr(s), l_cstr(s) + len};
}

L_INLINE l_strt
l_strt_sub(const void* s, l_int from, l_int to)
{
  return l_strt_from(l_cstr(s) + from, l_cstr(s) + to);
}

L_INLINE int
l_strt_isEmpty(const l_strt* s)
{
  return !(s->start != 0 && s->start < s->end);
}

L_INLINE l_strn
l_strt_strn(const l_strt* s)
{
  return (l_strn){s->start, s->end - s->start};
}

L_INLINE l_strn
l_strn_from(const void* s, const void* e)
{
  return (l_strn){l_cstr(s), l_cstr(e) - l_cstr(s)};
}

L_INLINE l_strn
l_strn_n(const void* s, l_int len)
{
  return (l_strn){l_cstr(s), len};
}

L_INLINE l_strn
l_strn_sub(const void* s, l_int from, l_int to)
{
  return l_strn_from(l_cstr(s) + from, l_cstr(s) + to);
}

L_INLINE int
l_strn_isEmpty(const l_strn* s)
{
  return !(s->start && s->len > 0);
}

L_INLINE l_strt
l_strn_strt(const l_strn* s)
{
  return l_strt_n(s->start, s->len);
}

#define l_zero(s, e) l_zero_n((s), l_cstr(e) - l_cstr(s))
#define l_copy(s, e, to) l_copy_n((s), l_cstr(e) - l_cstr(s), (to))

L_EXTERN void l_zero_n(void* s, l_int n);
L_EXTERN l_byte* l_copy_n(const void* s, l_int n, void* to);
L_EXTERN l_byte* l_copy_from(l_strt from, void* to);
L_EXTERN int l_strt_equal(l_strt a, l_strt b);
L_EXTERN int l_strn_equal(l_strn a, l_strn b);
L_EXTERN const l_byte* l_strt_contain(l_strt s, int ch);
L_EXTERN const l_byte* l_strn_contain(l_strn s, int ch);

/**
 * assert and logging
 */

#define L_MKSTR(a) #a
#define L_X_MKSTR(a) L_MKSTR(a)
#define L_MKFLSTR __FILE__ " (" L_X_MKSTR(__LINE__) ") "

#if defined(L_BUILD_DEBUG)
  #define L_DEBUG_HERE(...) { __VA_ARGS__ }
  #define l_assert_pass_func(expr) l_logger_func_impl("41[D] " L_MKFLSTR, "assert pass: %s", lp(expr))
  #define l_assert_fail_func(expr) l_logger_func_impl("01[E] " L_MKFLSTR, "assert fail: %s", lp(expr))
#else
  #define L_DEBUG_HERE(...)
  #define l_assert_pass_func(expr) ((void)0)
  #define l_assert_fail_func(expr) l_logger_func_impl("01[E] " L_MKFLSTR, "assert fail: %s", lp(expr))
#endif

#define l_assert(e) ((e) ? l_assert_pass_func(#e) : l_assert_fail_func(#e)) /* 0:assert */
#define l_loge_s(s)                   l_logger_func_s("10[E] " L_MKFLSTR, (s)) /* 1:error */
#define l_loge_1(fmt,a)               l_logger_func_1("11[E] " L_MKFLSTR, (fmt), a)
#define l_loge_n(fmt,n,a)             l_logger_func_n("1n[E] " L_MKFLSTR, (fmt), n,a)
#define l_loge_2(fmt,a,b)             l_logger_func_2("12[E] " L_MKFLSTR, (fmt), a,b)
#define l_loge_3(fmt,a,b,c)           l_logger_func_3("13[E] " L_MKFLSTR, (fmt), a,b,c)
#define l_loge_4(fmt,a,b,c,d)         l_logger_func_4("14[E] " L_MKFLSTR, (fmt), a,b,c,d)
#define l_loge_5(fmt,a,b,c,d,e)       l_logger_func_5("15[E] " L_MKFLSTR, (fmt), a,b,c,d,e)
#define l_loge_6(fmt,a,b,c,d,e,f)     l_logger_func_6("16[E] " L_MKFLSTR, (fmt), a,b,c,d,e,f)
#define l_loge_7(fmt,a,b,c,d,e,f,g)   l_logger_func_7("17[E] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g)
#define l_loge_8(fmt,a,b,c,d,e,f,g,h) l_logger_func_8("18[E] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g,h)
#define l_loge_9(t,a,b,c,d,e,f,g,h,i) l_logger_func_9("19[E] " L_MKFLSTR, (t), a,b,c,d,e,f,g,h,i)
#define l_logw_s(s)                   l_logger_func_s("20[W] " L_MKFLSTR, (s)) /* 2:warning */
#define l_logw_1(fmt,a)               l_logger_func_1("21[W] " L_MKFLSTR, (fmt), a)
#define l_logw_n(fmt,n,a)             l_logger_func_n("2n[W] " L_MKFLSTR, (fmt), n,a)
#define l_logw_2(fmt,a,b)             l_logger_func_2("22[W] " L_MKFLSTR, (fmt), a,b)
#define l_logw_3(fmt,a,b,c)           l_logger_func_3("23[W] " L_MKFLSTR, (fmt), a,b,c)
#define l_logw_4(fmt,a,b,c,d)         l_logger_func_4("24[W] " L_MKFLSTR, (fmt), a,b,c,d)
#define l_logw_5(fmt,a,b,c,d,e)       l_logger_func_5("25[W] " L_MKFLSTR, (fmt), a,b,c,d,e)
#define l_logw_6(fmt,a,b,c,d,e,f)     l_logger_func_6("26[W] " L_MKFLSTR, (fmt), a,b,c,d,e,f)
#define l_logw_7(fmt,a,b,c,d,e,f,g)   l_logger_func_7("27[W] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g)
#define l_logw_8(fmt,a,b,c,d,e,f,g,h) l_logger_func_8("28[W] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g,h)
#define l_logw_9(t,a,b,c,d,e,f,g,h,i) l_logger_func_9("29[W] " L_MKFLSTR, (t), a,b,c,d,e,f,g,h,i)
#define l_logm_s(s)                   l_logger_func_s("30[L] " L_MKFLSTR, (s)) /* 3:main flow */
#define l_logm_1(fmt,a)               l_logger_func_1("31[L] " L_MKFLSTR, (fmt), a)
#define l_logm_n(fmt,n,a)             l_logger_func_n("3n[L] " L_MKFLSTR, (fmt), n,a)
#define l_logm_2(fmt,a,b)             l_logger_func_2("32[L] " L_MKFLSTR, (fmt), a,b)
#define l_logm_3(fmt,a,b,c)           l_logger_func_3("33[L] " L_MKFLSTR, (fmt), a,b,c)
#define l_logm_4(fmt,a,b,c,d)         l_logger_func_4("34[L] " L_MKFLSTR, (fmt), a,b,c,d)
#define l_logm_5(fmt,a,b,c,d,e)       l_logger_func_5("35[L] " L_MKFLSTR, (fmt), a,b,c,d,e)
#define l_logm_6(fmt,a,b,c,d,e,f)     l_logger_func_6("36[L] " L_MKFLSTR, (fmt), a,b,c,d,e,f)
#define l_logm_7(fmt,a,b,c,d,e,f,g)   l_logger_func_7("37[L] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g)
#define l_logm_8(fmt,a,b,c,d,e,f,g,h) l_logger_func_8("38[L] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g,h)
#define l_logm_9(t,a,b,c,d,e,f,g,h,i) l_logger_func_9("39[L] " L_MKFLSTR, (t), a,b,c,d,e,f,g,h,i)
#define l_logd_s(s)                   l_logger_func_s("40[D] " L_MKFLSTR, (s)) /* 4:debug log */
#define l_logd_1(fmt,a)               l_logger_func_1("41[D] " L_MKFLSTR, (fmt), a)
#define l_logd_n(fmt,n,a)             l_logger_func_n("4n[D] " L_MKFLSTR, (fmt), n,a)
#define l_logd_2(fmt,a,b)             l_logger_func_2("42[D] " L_MKFLSTR, (fmt), a,b)
#define l_logd_3(fmt,a,b,c)           l_logger_func_3("43[D] " L_MKFLSTR, (fmt), a,b,c)
#define l_logd_4(fmt,a,b,c,d)         l_logger_func_4("44[D] " L_MKFLSTR, (fmt), a,b,c,d)
#define l_logd_5(fmt,a,b,c,d,e)       l_logger_func_5("45[D] " L_MKFLSTR, (fmt), a,b,c,d,e)
#define l_logd_6(fmt,a,b,c,d,e,f)     l_logger_func_6("46[D] " L_MKFLSTR, (fmt), a,b,c,d,e,f)
#define l_logd_7(fmt,a,b,c,d,e,f,g)   l_logger_func_7("47[D] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g)
#define l_logd_8(fmt,a,b,c,d,e,f,g,h) l_logger_func_8("48[D] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g,h)
#define l_logd_9(t,a,b,c,d,e,f,g,h,i) l_logger_func_9("49[D] " L_MKFLSTR, (t), a,b,c,d,e,f,g,h,i)

#define ls(s) lp(s)
#define lc(a) ld(a)
#define lt(a) ld(a)
#define lb(a) lu(a)
#define lo(a) lu(a)
#define lx(a) lu(a)
#define lserror(n) lp(strerror(n))

typedef union {
  l_long d;
  l_ulong u;
  double f;
  const void* p;
} l_value;

L_INLINE l_value
lp(const void* p)
{
  l_value a; a.p = p; return a;
}

L_INLINE l_value
ld(l_long d)
{
  l_value a; a.d = d; return a;
}

L_INLINE l_value
lu(l_ulong u)
{
  l_value a; a.u = u; return a;
}

L_INLINE l_value
lf(double f)
{
  l_value a; a.f = f; return a;
}

L_INLINE l_value
lstrt(const l_strt* s)
{
  return lp(s);
}

L_INLINE l_value
lstrn(const l_strn* s)
{
  return lp(s);
}

L_EXTERN int l_logger_getLevel();
L_EXTERN void l_logger_setLevel(int n);
L_EXTERN void l_logger_flush();
L_PRIVAT void l_logger_func_impl(const void* tag, const void* fmt, ...);

L_INLINE void
l_logger_func_s(const void* tag, const void* s)
{
  l_logger_func_impl(tag, s, 0);
}

L_INLINE void
l_logger_func_1(const void* tag, const void* s, l_value a)
{
  l_logger_func_impl(tag, s, a);
}

L_INLINE void
l_logger_func_2(const void* tag, const void* s, l_value a, l_value b)
{
  l_logger_func_impl(tag, s, a, b);
}

L_INLINE void
l_logger_func_3(const void* tag, const void* s, l_value a, l_value b, l_value c)
{
  l_logger_func_impl(tag, s, a, b, c);
}

L_INLINE void
l_logger_func_4(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d)
{
  l_logger_func_impl(tag, s, a, b, c, d);
}

L_INLINE void
l_logger_func_5(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d,
    l_value e)
{
  l_logger_func_impl(tag, s, a, b, c, d, e);
}

L_INLINE void
l_logger_func_6(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d,
    l_value e, l_value f)
{
  l_logger_func_impl(tag, s, a, b, c, d, e, f);
}

L_INLINE void
l_logger_func_7(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d,
    l_value e, l_value f, l_value g)
{
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g);
}

L_INLINE void
l_logger_func_8(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d,
    l_value e, l_value f, l_value g, l_value h)
{
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g, h);
}

L_INLINE void
l_logger_func_9(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d,
    l_value e, l_value f, l_value g, l_value h, l_value i)
{
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g, h, i);
}

L_INLINE void
l_logger_func_n(const void* tag, const void* s, l_int n, const l_value* a)
{
  l_logger_func_impl(tag, s, n, a);
}

/**
 * simple link list
 */

typedef struct l_smplnode {
  struct l_smplnode* next;
} l_smplnode;

L_INLINE void
l_smplnode_init(l_smplnode* node)
{
  node->next = node;
}

L_INLINE int
l_smplnode_isEmpty(l_smplnode* node)
{
  return node->next == node;
}

L_INLINE void
l_smplnode_insertAfter(l_smplnode* node, l_smplnode* newnode)
{
  newnode->next = node->next;
  node->next = newnode;
}

L_INLINE l_smplnode*
l_smplnode_removeNext(l_smplnode* node)
{
  l_smplnode* p = node->next;
  node->next = p->next;
  return p;
}

/**
 * bidirectional link list
 */

typedef struct l_linknode {
  struct l_linknode* next;
  struct l_linknode* prev;
} l_linknode;

L_INLINE void
l_linknode_init(l_linknode* node)
{
  node->next = node->prev = node;
}

L_INLINE int
l_linknode_isEmpty(l_linknode* node)
{
  return (node->next == node);
}

L_INLINE void
l_linknode_insertAfter(l_linknode* node, l_linknode* newnode)
{
  newnode->next = node->next;
  node->next = newnode;
  newnode->prev = node;
  newnode->next->prev = newnode;
}

L_INLINE l_linknode*
l_linknode_remove(l_linknode* node)
{
  node->prev->next = node->next;
  node->next->prev = node->prev;
  return node;
}

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
  l_long sec;
  l_umedit nsec;
} l_time;

typedef struct {
  l_umedit year; /* 38-bit, can represent 274877906943 years */
  l_umedit nsec; /* nanoseconds that less than 1 sec */
  l_ushort yday; /* 1 ~ 366 */
  l_byte high;   /* extra bits for year */
  l_byte wdmon;  /* 1~12, high 4-bit is wday (0~6, 0 is sunday) */
  l_byte day;    /* 1~31 */
  l_byte hour;   /* 0~23 */
  l_byte min;    /* 0~59 */
  l_byte sec;    /* 0~61, 60 and 61 are the leap seconds */
} l_date;

L_EXTERN l_time l_time_system();
L_EXTERN l_time l_time_monotonic();
L_EXTERN l_date l_date_system();
L_EXTERN l_date l_date_fromUtcSecs(l_long utcsecs);
L_EXTERN l_date l_date_fromUtcTime(l_time utc);

L_INLINE l_umedit
l_right_most_bit(l_umedit n)
{
  return n & (-n);
}

L_EXTERN void l_process_exit();
L_EXTERN void l_core_base_test();

#endif /* lucy_core_base_h */

