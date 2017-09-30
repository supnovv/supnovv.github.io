#ifndef lucy_core_base_h
#define lucy_core_base_h
#include "autoconf.h"
#include "core/prefix.h"

#undef l_spec_extern
#undef l_spec_specif
#undef l_spec_inline
#undef l_spec_static
#undef l_spec_shared
#undef l_spec_thread_local
#undef l_spec_thread_local_decl

#define l_spec_extern(a) extern a
#define l_spec_specif(a) extern a
#define l_spec_inline(a) static a
#define l_spec_static(a) static a

#if defined(__GNUC__)
  #define l_spec_shared(a) extern a
#else
  #if defined(L_CORELIB_IMPL) || defined(L_WINDOWS_IMPL)
    #define l_spec_shared(a) __declspec(dllexport) a
  #else
    #define l_spec_shared(a) __declspec(dllimport) a
  #endif
#endif

#if defined(l_cmpl_gcc)
  #define l_spec_thread_local(a) __thread a
  #define l_spec_thread_local_decl(a) extern __thread a
#elif defined(l_cmpl_msc)
  #define l_spec_thread_local(a) __declspec(thread) a
  #define l_spec_thread_local_decl(a) extern __declspec(thread) a
#endif

#if defined(L_BUILD_DEBUG)
  #define l_debug_here(...) { __VA_ARGS__ }
#else
  #define l_debug_here(...)
#endif

#define l_cast(type, a) ((type)(a))
#define l_cstr(s) ((l_byte*)(s))

#define l_max_rwsize    (0x7fff0000) /* 2147418112 */
#define l_max_ubyte     l_cast(l_byte, 0xff) /* 255 */
#define l_max_ushort    l_cast(l_ushort, 0xffff) /* 65535 */
#define l_max_umedit    l_cast(l_umedit, 0xffffffff) /* 4294967295 */
#define l_max_ulong     l_cast(l_ulong, 0xffffffffffffffff) /* 18446744073709551615 */
#define l_max_sbyte     l_cast(l_sbyte, 0x7f) /* 127 */
#define l_max_short     l_cast(l_short, 0x7fff) /* 32767 */
#define l_max_medit     l_cast(l_medit, 0x7fffffff) /* 2147483647 */
#define l_max_long      l_cast(l_long, 0x7fffffffffffffff) /* 9223372036854775807 */
#define l_min_sbyte     l_cast(l_sbyte, -127-1) /* 128 0x80 */
#define l_min_short     l_cast(l_short, -32767-1) /* 32768 0x8000 */
#define l_min_medit     l_cast(l_medit, -2147483647-1) /* 2147483648 0x80000000 */
#define l_min_long      l_cast(l_long, -9223372036854775807-1) /* 9223372036854775808 0x8000000000000000 */

#define l_status_contread (3)
#define l_status_waitmore (2)
#define l_status_luayield (1)
#define l_status_success  (0)
#define l_status_luaerr   (-1)
#define l_status_error    (-2)
#define l_status_eread    (-3)
#define l_status_ewrite   (-4)
#define l_status_elimit   (-5)
#define l_status_ematch   (-6)
#define l_status_einval   (-7)

/**
 * memory operations
 */

#define l_malloc(allocfunc, ud, size) allocfunc((ud), 0, (size), 0)
#define l_calloc(allocfunc, ud, size) allocfunc((ud), 0, (size), 1)
#define l_ralloc(allocfunc, ud, buffer, oldsize, newsize) allocfunc((ud), (buffer), (oldsize), (newsize))
#define l_mfree(allocfunc, ud, buffer) allocfunc((ud), (buffer), 0, 0)

#define l_malloc(allocfunc, ud, size) allocfunc((ud), 0, (size), 0)
#define l_calloc(allocfunc, ud, size) allocfunc((ud), 0, (size), 1)
#define l_ralloc(allocfunc, ud, buffer, oldsize, newsize) allocfunc((ud), (buffer), (oldsize), (newsize))
#define l_mfree(allocfunc, ud, buffer) allocfunc((ud), (buffer), 0, 0)

#define l_raw_malloc(size) l_malloc(l_raw_alloc_func, 0, size)
#define l_raw_calloc(size) l_calloc(l_raw_alloc_func, 0, size)
#define l_raw_ralloc(buffer, oldsize, newsize) l_ralloc(l_raw_alloc_func, 0, buffer, oldsize, newsize)
#define l_raw_mfree(buffer) l_mfree(l_raw_alloc_func, 0, buffer)

typedef void* (*l_allocfunc)(void* userdata, void* buffer, l_int oldsize, l_int newsize);

l_spec_extern(void*)
l_raw_alloc_func(void* userdata, void* buffer, l_int oldsize, l_int newsize);

/**
 * simple link list
 */

typedef struct l_smplnode {
  struct l_smplnode* next;
} l_smplnode;

l_spec_inline(void)
l_smplnode_init(l_smplnode* node) {
  node->next = node;
}

l_spec_inline(int)
l_smplnode_isEmpty(l_smplnode* node) {
  return node->next == node;
}

l_spec_inline(void)
l_smplnode_insertAfter(l_smplnode* node, l_smplnode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
}

l_spec_inline(l_smplnode*)
l_smplnode_removeNext(l_smplnode* node) {
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

l_spec_inline(void)
l_linknode_init(l_linknode* node) {
  node->next = node->prev = node;
}

l_spec_inline(int)
l_linknode_isEmpty(l_linknode* node) {
  return (node->next == node);
}

l_spec_inline(void)
l_linknode_insertAfter(l_linknode* node, l_linknode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
  newnode->prev = node;
  newnode->next->prev = newnode;
}

l_spec_inline(l_linknode*)
l_linknode_remove(l_linknode* node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  return node;
}

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

l_spec_inline(l_strt)
l_strt_from(const void* s, const void* e) {
  return (l_strt){l_cstr(s), l_cstr(e)};
}

l_spec_inline(l_strt)
l_strt_n(const void* s, l_int len) {
  return (l_strt){l_cstr(s), l_cstr(s) + len};
}

l_spec_inline(l_strt)
l_strt_substr(const void* s, l_int from, l_int to) {
  return l_strt_from(l_cstr(s) + from, l_cstr(s) + to);
}

l_spec_inline(int)
l_strt_isEmpty(const l_strt* s) {
  return !(s->start != 0 && s->start < s->end);
}

l_spec_inline(l_strn)
l_strt_tostrn(const l_strt* s) {
  return (l_strn){s->start, s->end - s->start};
}

l_spec_inline(l_strn)
l_strn_from(const void* s, const void* e) {
  return (l_strn){l_cstr(s), l_cstr(e) - l_cstr(s)};
}

l_spec_inline(l_strn)
l_strn_n(const void* s, l_int len) {
  return (l_strn){l_cstr(s), len};
}

l_spec_inline(l_strn)
l_strn_substr(const void* s, l_int from, l_int to) {
  return l_strn_from(l_cstr(s) + from, l_cstr(s) + to);
}

l_spec_inline(int)
l_strn_isEmpty(const l_strn* s) {
  return !(s->start && s->len > 0);
}

l_spec_inline(l_strt)
l_strn_tostrt(const l_strn* s) {
  return l_strt_n(s->start, s->len);
}

#define l_zero(s, e) l_zero_n((s), l_cstr(e) - l_cstr(s))
#define l_copy(s, e, to) l_copy_n((s), l_cstr(e) - l_cstr(s), (to))

l_spec_extern(void)
l_zero_n(void* s, l_int n);

l_spec_extern(l_byte*)
l_copy_n(const void* s, l_int n, void* to);

l_spec_extern(l_byte*)
l_copy_from(l_strt from, void* to);

l_spec_extern(int)
l_strt_equal(l_strt a, l_strt b);

l_spec_extern(int)
l_strn_equal(l_strn a, l_strn b);

l_spec_extern(const l_byte*)
l_strt_contain(l_strt s, int ch);

l_spec_extern(const l_byte*)
l_strn_contain(l_strn s, int ch);

/**
 * time and date
 *
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

l_spec_specif(l_time)
l_time_system();

l_spec_specif(l_time)
l_time_monotonic();

l_spec_specif(l_date)
l_date_system();

l_spec_specif(l_date)
l_date_fromsecs(l_long utcsecs);

l_spec_specif(l_date)
l_date_fromtime(l_time utc);

/**
 * assert and logging
 */

#define L_MKSTR(a) #a
#define L_X_MKSTR(a) L_MKSTR(a)
#define L_MKFLSTR __FILE__ " (" L_X_MKSTR(__LINE__) ") "

#define l_assert(e) ((e) ? l_assert_pass_func("41[D] " L_MKFLSTR, (#e)) : l_assert_fail_func("01[E] " L_MKFLSTR, (#e))) /* 0:assert */
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

typedef union {
  l_long d;
  l_ulong u;
  double f;
  const void* p;
} l_value;

l_spec_inline(l_value)
lp(const void* p) {
  l_value a; a.p = p; return a;
}

l_spec_inline(l_value)
ld(l_long d) {
  l_value a; a.d = d; return a;
}

l_spec_inline(l_value)
lu(l_ulong u) {
  l_value a; a.u = u; return a;
}

l_spec_inline(l_value)
lf(double f) {
  l_value a; a.f = f; return a;
}

l_spec_inline(l_value)
lstrt(const l_strt* s) {
  return lp(s);
}

l_spec_inline(l_value)
lstrn(const l_strn* s) {
  return lp(s);
}

l_spec_extern(void)
l_assert_pass_func(const void* tag, const void* expr);

l_spec_extern(void)
l_assert_fail_func(const void* tag, const void* expr);

l_spec_extern(void)
l_logger_func_impl(const void* tag, const void* fmt, ...);

l_spec_inline(void)
l_logger_func_s(const void* tag, const void* s) {
  l_logger_func_impl(tag, s, 0);
}

l_spec_inline(void)
l_logger_func_1(const void* tag, const void* s, l_value a) {
  l_logger_func_impl(tag, s, a);
}

l_spec_inline(void)
l_logger_func_2(const void* tag, const void* s, l_value a, l_value b) {
  l_logger_func_impl(tag, s, a, b);
}

l_spec_inline(void)
l_logger_func_3(const void* tag, const void* s, l_value a, l_value b, l_value c) {
  l_logger_func_impl(tag, s, a, b, c);
}

l_spec_inline(void)
l_logger_func_4(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d) {
  l_logger_func_impl(tag, s, a, b, c, d);
}

l_spec_inline(void)
l_logger_func_5(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d, l_value e) {
  l_logger_func_impl(tag, s, a, b, c, d, e);
}

l_spec_inline(void)
l_logger_func_6(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d, l_value e, l_value f) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f);
}

l_spec_inline(void)
l_logger_func_7(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d, l_value e, l_value f, l_value g) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g);
}

l_spec_inline(void)
l_logger_func_8(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g, h);
}

l_spec_inline(void)
l_logger_func_9(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g, h, i);
}

l_spec_inline(void)
l_logger_func_n(const void* tag, const void* s, l_int n, const l_value* a) {
  l_logger_func_impl(tag, s, n, a);
}

l_spec_extern(void)
l_logger_setLevel(int level);

l_spec_extern(int)
l_logger_getLevel();

l_spec_extern(void)
l_process_exit();

l_spec_extern(void)
l_core_base_test();

#endif /* lucy_core_base_h */

