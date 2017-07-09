#ifndef l_core_lib_h
#define l_core_lib_h
#include "l_prefix.h"
#include "autoconf.h"

#undef l_inline
#define l_inline static

#undef l_extern
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

#undef l_specif
#define l_specif l_extern

#undef l_thread_local
#undef l_extern_thread_local
#undef l_thread_local_supported
#if defined(L_CLER_GCC)
  #define l_thread_local(a) __thread a
  #define l_extern_thread_local(a) extern __thread a
  #define l_thread_local_supported
#elif defined(L_CLER_MSC)
  #define l_thread_local(a) __declspec(thread) a
  #define l_extern_thread_local(a) extern __declspec(thread) a
  #define l_thread_local_supported
#else
  #define l_thread_local(a)
#endif

#define l_cast(type, a) ((type)(a))
#define l_rstr(s) ((l_rune*)(s))

#define l_max_rdwr_size (0x7fff0000) /* 2147418112 */
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

#define L_STATUS_CONTREAD (3)
#define L_STATUS_WAITMORE (2)
#define L_STATUS_YIELD (1)
#define L_STATUS_OK (0)
#define L_STATUS_LUAERR (-1)
#define L_STATUS_ERROR (-2)
#define L_STATUS_EREAD (-3)
#define L_STATUS_EWRITE (-4)
#define L_STATUS_ELIMIT (-5)
#define L_STATUS_EMATCH (-6)
#define L_STATUS_EINVAL (-7)

#ifdef L_BUILD_DEBUG
#define L_DEBUG_HERE(...) { __VA_ARGS__ }
#define l_debug_assert(e) l_assert(e)
#else
#define L_DEBUG_HERE(...) { ((void)0); }
#define l_debug_assert(e) ((void)0)
#endif

#define L_MKSTR(a) #a
#define L_X_MKSTR(a) L_MKSTR(a)
#define L_MKFLSTR __FILE__ " (" L_X_MKSTR(__LINE__) ") "

#define l_assert(e) ((e) ? l_assert_func_pass("41[D] " L_MKFLSTR, (#e)) : l_assert_func_fail("01[E] " L_MKFLSTR, (#e))) /* 0:assert */
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

#define l_zero_e(start, end) l_zero_l(start, l_str(end) - l_str(start))
l_extern void l_zero_l(void* start, l_int len);

#define l_copy_e(from, end, to) return l_copy_l(from, l_str(end) - l_str(from), (to))
l_extern l_byte* l_copy_l(const void* from, l_int len, void* to);

l_extern void* l_raw_malloc(l_int size);
l_extern void* l_raw_calloc(l_int size);
l_extern void* l_raw_realloc(void* buffer, l_int oldsize, l_int newsize);
l_extern void l_raw_free(void* buffer);

typedef union {
  l_long d;
  l_ulong u;
  double f;
  const void* p;
} l_value;

#define ls(s) lp(s)
#define lserror(n) lp(strerror(n))
#define lstring(s) lp(l_string_cstr(s))

l_inline l_value lp(const void* p) {
  l_value a; a.p = p; return a;
}

l_inline l_value ld(l_long d) {
  l_value a; a.d = d; return a;
}

l_inline l_value lu(l_ulong u) {
  l_value a; a.u = u; return a;
}

l_inline l_value lf(double f) {
  l_value a; a.f = f; return a;
}

l_extern void l_assert_func_pass(const void* tag, const void* expr);
l_extern void l_assert_func_fail(const void* tag, const void* expr);
l_extern void l_logger_func_impl(const void* tag, const void* fmt, ...);
l_extern void l_set_log_level(int level);
l_extern int l_get_log_level();

l_inline void l_logger_func_s(const void* tag, const void* s) {
  l_logger_func_impl(tag, s, 0);
}

l_inline void l_logger_func_1(const void* tag, const void* s, l_value a) {
  l_logger_func_impl(tag, s, a);
}

l_inline void l_logger_func_2(const void* tag, const void* s, l_value a,
    l_value b) {
  l_logger_func_impl(tag, s, a, b);
}

l_inline void l_logger_func_3(const void* tag, const void* s, l_value a,
    l_value b, l_value c) {
  l_logger_func_impl(tag, s, a, b, c);
}

l_inline void l_logger_func_4(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d) {
  l_logger_func_impl(tag, s, a, b, c, d);
}

l_inline void l_logger_func_5(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e) {
  l_logger_func_impl(tag, s, a, b, c, d, e);
}

l_inline void l_logger_func_6(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f);
}

l_inline void l_logger_func_7(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g);
}

l_inline void l_logger_func_8(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g, h);
}

l_inline void l_logger_func_9(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g, h, i);
}

l_inline void l_logger_func_n(const void* tag, const void* s, l_int n, const l_value* a) {
  l_logger_func_impl(tag, s, n, a);
}

typedef struct {
  const l_byte* start;
  const l_byte* end;
} l_strt;

#define l_strt_c(s) l_strt_l((s), strlen((char*)(s)))
#define l_literal_strt(s) l_strt_l("" s, (sizeof(s)/sizeof(char))-1)
#define l_empty_strt() ((l_strt){0,0})

l_inline l_strt l_strt_e(const void* s, const void* e) {
  return (l_strt){l_rstr(s), l_rstr(e)};
}

l_inline l_strt l_strt_l(const void* s, l_int len) {
  return (l_strt){l_rstr(s), l_rstr(s) + len};
}

l_inline l_strt l_strt_sft(const void* s, l_int from, l_int to) {
  return l_strt_e(l_rstr(s) + from, l_rstr(s) + to);
}

l_extern int l_strt_equal(l_strt l, l_strt r);
l_extern int l_strt_contain(l_strt s, int ch);
l_extern l_byte* l_copy_from(l_strt s, void* to);

typedef struct {
  void* stream;
} l_filestream;

l_extern l_filestream l_open_read(const void* name);
l_extern l_filestream l_open_write(const void* name);
l_extern l_filestream l_open_append(const void* name);
l_extern l_filestream l_open_read_write(const void* name);
l_extern l_filestream l_open_write_unbuffered(const void* name);
l_extern l_filestream l_open_append_unbuffered(const void* name);
l_extern l_int l_write_strt_to_file(l_filestream* self, l_strt s);
l_extern l_int l_write_file(l_filestream* self, const void* s, l_int len);
l_extern l_int l_read_file(l_filestream* self, void* out, l_int len);

l_extern int l_remove_file(const void* name);
l_extern int l_rename_file(const void* from, const void* to);
l_extern void l_redirect_stdout(const void* name);
l_extern void l_redirect_stderr(const void* name);
l_extern void l_reditect_stdin(const void* name);
l_extern void l_close_file(l_filestream* self);
l_extern void l_flush_file(l_filestream* self);
l_extern void l_rewind_file(l_filestream* self);
l_extern void l_seek_from_begin(l_filestream* self, long offset);
l_extern void l_seek_from_curpos(l_filestream* self, long offset);
l_extern void l_clear_file_error(l_filestream* self);

typedef struct l_linknode {
  struct l_linknode* next;
  struct l_linknode* prev;
} l_linknode;

typedef struct l_smplnode {
  struct l_smplnode* next;
} l_smplnode;

l_extern void l_linknode_init(l_linknode* node);
l_extern void l_linknode_insert_after(l_linknode* node, l_linknode* newnode);
l_extern int l_linknode_is_empty(l_linknode* node);
l_extern l_linknode* l_linknode_remove(l_linknode* node);

l_extern void l_smplnode_init(l_smplnode* node);
l_extern void l_smplnode_insert_after(l_smplnode* node, l_smplnode* newnode);
l_extern int l_smplnode_is_empty(l_smplnode* node);
l_extern l_smplnode* l_smplnode_remove_next(l_smplnode* node);

typedef struct l_squeue {
  l_smplnode head;
  l_smplnode* tail;
} l_squeue;

typedef struct l_dqueue {
  l_linknode head;
} l_dqueue;

l_extern void l_squeue_init(l_squeue* self);
l_extern void l_squeue_push(l_squeue* self, l_smplnode* newnode);
l_extern void l_squeue_push_queue(l_squeue* self, l_squeue* queue);
l_extern int l_squeue_is_empty(l_squeue* self);
l_extern l_smplnode* l_squeue_pop(l_squeue* self);

l_extern void l_dqueue_init(l_dqueue* self);
l_extern void l_dqueue_push(l_dqueue* self, l_linknode* newnode);
l_extern void l_dqueue_push_queue(l_dqueue* self, l_dqueue* queue);
l_extern int l_dqueue_is_empty(l_dqueue* self);
l_extern l_linknode* l_dqueue_pop(l_dqueue* self);

typedef struct l_priorq {
  l_linknode node;
  /* elem with less number has higher priority, i.e., 0 is the highest */
  int (*less)(void* elem_is_less_than, void* this_one);
} l_priorq;

l_extern void l_priorq_init(l_priorq* self, int (*less)(void*, void*));
l_extern void l_priorq_push(l_priorq* self, l_linknode* elem);
l_extern void l_priorq_remove(l_priorq* self, l_linknode* elem);
l_extern int l_priorq_is_empty(l_priorq* self);
l_extern l_linknode* l_priorq_pop(l_priorq* self);

/* min/max heap - add/remove quick, search slow */
typedef struct {
  l_umedit size;
  l_umedit capacity;
  l_int* a; /* array of elements with pointer size */
  int (*less)(void* this_elem_is_less, void* than_this_one);
} l_mmheap;

/* min. heap - pass less function to less, max. heap - pass greater function to less */
l_extern void l_mmheap_init(l_mmheap* self, int (*less)(void*, void*), int initsize);
l_extern void l_mmheap_free(l_mmheap* self);
l_extern void l_mmheap_add(l_mmheap* self, void* elem);
l_extern void* l_mmheap_del(l_mmheap* self, l_umedit i);

#define L_COMMON_BUFHEAD \
  l_smplnode node; \
  l_int bsize;

typedef struct {
  L_COMMON_BUFHEAD
  l_ushort flags;
  l_ushort nargs;
  l_int size;
  l_int limit;
} l_strbuf;

typedef struct {
  l_strbuf* b;
} l_string;

typedef struct l_thread l_thread;
l_extern l_string l_create_string(l_int initsize);
l_extern l_string l_create_string_from(l_strt from);
l_extern l_string l_create_limited_string(l_int initsize, l_int maxlimit);
l_extern l_string l_create_limited_string_from(l_strt from, l_int maxlimit);
l_extern l_string l_thread_create_string(l_thread* t, l_int initsize);
l_extern l_string l_thread_create_string_from(l_thread* t, l_strt from);
l_extern l_string l_thread_create_limited_string(l_thread* t, l_int initsize, l_int maxlimit);
l_extern l_string l_thread_create_limited_string_from(l_thread* t, l_strt from, l_int maxlimit);
l_extern void l_thread_string_free(l_thread* thread, l_string* self);
l_extern void l_string_free(l_string* self);
l_extern void l_string_clear(l_string* self);
l_extern void l_string_set(l_string* self, l_strt s);
l_extern int l_string_ensure_capacity(l_string* self, l_int capacity);
l_extern int l_string_ensure_remain(l_string* self, l_int remainsize);
l_extern int l_string_append(l_string* self, l_strt s);
l_extern int l_string_format_impl(l_string* self, const void* fmt, ...);
l_extern int l_string_format_n(l_string* self, const void* fmt, int n, l_value* a);

l_inline l_byte* l_string_cstr(l_string* self) {
  return (l_byte*)(self->b + 1);
}

l_inline l_int l_string_capacity(l_string* self) {
  return self->b->bsize - sizeof(l_strbuf);
}

l_inline l_int l_string_size(l_string* self) {
  return self->b->size;
}

l_inline l_byte* l_string_start(l_string* self) {
  return l_string_cstr(self);
}

l_inline l_byte* l_string_end(l_string* self) {
  return l_string_cstr(self) + l_string_size(self);
}

l_inline l_strt l_string_strt(l_string* self) {
  return l_strt_l(l_string_start(self), self->b->size);
}

l_inline int l_string_is_empty(l_string* self) {
  return self->b->size == 0;
}

l_inline int l_string_nt_empty(l_string* self) {
  return self->b->size != 0;
}

l_inline l_int l_string_remain(l_string* self) {
  return l_string_capacity(self) - l_string_size(self);
}

l_inline int l_string_equal(l_string* self, l_strt s) {
  return l_strt_equal(l_string_strt(self), s);
}

l_inline int l_string_format_1(l_string* self, const void* fmt, l_value a) {
  self->b->nargs = 1;
  return l_string_format_impl(self, fmt, a);
}

l_inline int l_string_format_2(l_string* self, const void* fmt, l_value a,
    l_value b) {
  self->b->nargs = 2;
  return l_string_format_impl(self, fmt, a, b);
}

l_inline int l_string_format_3(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c) {
  self->b->nargs = 3;
  return l_string_format_impl(self, fmt, a, b, c);
}

l_inline int l_string_format_4(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d) {
  self->b->nargs = 4;
  return l_string_format_impl(self, fmt, a, b, c, d);
}

l_inline int l_string_format_5(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e) {
  self->b->nargs = 5;
  return l_string_format_impl(self, fmt, a, b, c, d, e);
}

l_inline int l_string_format_6(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f) {
  self->b->nargs = 6;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f);
}

l_inline int l_string_format_7(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g) {
  self->b->nargs = 7;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g);
}

l_inline int l_string_format_8(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h) {
  self->b->nargs = 8;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g, h);
}

l_inline int l_string_format_9(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i) {
  self->b->nargs = 9;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g, h, i);
}

typedef struct {
  void* t; /* table array, 1 table contains 1 rune, the array size is up to the length of the string */
  l_int size; /* a string map can store strings up to 'maxnumofstr', the size is the length of the longest string */
  l_int maxnumofstr;
} l_stringmap;

l_extern const l_stringmap* l_string_space_map();
l_extern const l_stringmap* l_string_newline_map();
l_extern const l_stringmap* l_string_blank_map();

l_extern l_stringmap l_string_new_map(l_int maxstrlen, const l_rune** str, l_int numofstr, int casesensitive);
l_extern void l_string_set_map(l_stringmap* self, const l_rune** str, l_int numofstr, int casesensitive);
l_extern void l_string_free_map(l_stringmap* self);

l_extern const l_rune* l_string_match(const l_stringmap* map, l_strt s);
l_extern const l_rune* l_string_match_ex(const l_stringmap* map, l_strt s, l_int* strid, l_int* mlen);
l_extern const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, l_strt s);
l_extern const l_rune* l_string_match_repeat(const l_stringmap* map, l_strt s);
l_extern const l_rune* l_string_match_until(const l_stringmap* map, l_strt s, l_rune** last_match_start);
l_extern const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, l_strt s, l_rune** first_non_space_pos);
l_extern const l_rune* l_string_skip_space_and_match(const l_stringmap* map, l_strt s, l_int* strid, l_int* mlen);

l_extern const l_rune* l_string_trim_head(l_strt s);
l_extern const l_rune* l_string_skip_space_and_match_sub(l_strt sub, l_strt s);

l_extern const l_rune l_rune_class_table[256];

l_inline int l_check_rune_is_pritable(l_byte ch) {
  return l_rune_class_table[ch];
}

l_inline int l_check_digit_is_dec(l_byte ch) {
  return l_rune_class_table[ch] == 5;
}

l_inline int l_check_digit_is_hex(l_byte ch) {
  return l_rune_class_table[ch] & 0x04;
}

l_inline int l_check_rune_is_letter(l_byte ch) {
  return l_rune_class_table[ch] & 0x02;
}

l_inline int l_check_upper_letter(l_byte ch) {
  return (l_rune_class_table[ch] & 0x03) == 2;
}

l_inline int l_check_lower_letter(l_byte ch) {
  return (l_rune_class_table[ch] & 0x03) == 3;
}

l_extern l_int l_string_parse_dec(l_strt s);
l_extern l_int l_string_parse_hex(l_strt s);

/* linuxcore.c */

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

l_specif l_time l_system_time();
l_specif l_time l_monotonic_time();
l_specif l_date l_system_date();
l_specif l_date l_date_from_secs(l_long utcsecs);
l_specif l_date l_date_from_time(l_time utc);

typedef struct {
  l_long fsize;
  l_long ctm; /* creation, utc */
  l_long atm; /* last access, utc */
  l_long mtm; /* last modify, utc */
  l_long gid;
  l_long uid;
  l_long mode;
  l_byte isfile;
  l_byte isdir;
  l_byte islink;
} l_fileattr;

typedef struct {
  void* stream;
} l_dirstream;

l_specif l_long l_file_size(const void* name);
l_specif l_fileattr l_file_attr(const void* name);
l_specif l_dirstream l_open_dir(const void* name);
l_specif void l_close_dir(l_dirstream* d);
l_specif const l_rune* l_read_dir(l_dirstream* d);

typedef struct {
  L_PLAT_IMPL_SIZE(L_MUTEX_SIZE);
} l_mutex;

typedef struct {
  L_PLAT_IMPL_SIZE(L_RWLOCK_SIZE);
} l_rwlock;

typedef struct {
  L_PLAT_IMPL_SIZE(L_CONDV_SIZE);
} l_condv;

typedef struct {
  L_PLAT_IMPL_SIZE(L_THRID_SIZE);
} l_thrid;

typedef struct {
  L_PLAT_IMPL_SIZE(L_THKEY_SIZE);
} l_thrkey;

l_specif void l_thrkey_init(l_thrkey* self);
l_specif void l_thrkey_free(l_thrkey* self);
l_specif void l_thrkey_set_data(l_thrkey* self, const void* data);
l_specif void* l_thrkey_get_data(l_thrkey* self);

l_specif void l_mutex_init(l_mutex* self);
l_specif void l_mutex_free(l_mutex* self);
l_specif void l_mutex_lock(l_mutex* self);
l_specif void l_mutex_unlock(l_mutex* self);
l_specif int l_mutex_trylock(l_mutex* self);

l_specif void l_rwlock_init(l_rwlock* self);
l_specif void l_rwlock_free(l_rwlock* self);
l_specif void l_rwlock_read(l_rwlock* self);
l_specif void l_rwlock_write(l_rwlock* self);
l_specif int l_rwlock_tryread(l_rwlock* self);
l_specif int l_rwlock_trywrite(l_rwlock* self);
l_specif void l_rwlock_unlock(l_rwlock* self);

l_specif void l_condv_init(l_condv* self);
l_specif void l_condv_free(l_condv* self);
l_specif void l_condv_wait(l_condv* self, l_mutex* mutex);
l_specif int l_condv_timedwait(l_condv* self, l_mutex* mutex, l_long ns);
l_specif void l_condv_signal(l_condv* self);
l_specif void l_condv_broadcast(l_condv* self);

l_specif l_thrid l_raw_thread_self();
l_specif int l_raw_thread_create(l_thrid* thrid, void* (*start)(void*), void* para);
l_specif int l_raw_thread_join(l_thrid* thrid);
l_specif void l_raw_thread_cancel(l_thrid* thrid);
l_specif void l_raw_thread_exit();
l_specif void l_thread_sleep(l_long us);

typedef struct lua_State lua_State;
typedef struct l_service l_service;

typedef struct l_thread {
  l_linknode node;
  l_umedit weight;
  l_ushort index;
  /* shared with master */
  l_mutex* svmtx;
  l_mutex* mutex;
  l_condv* condv;
  l_squeue* rxmq;
  int msgwait;
  /* thread own use */
  lua_State* L;
  l_squeue* txmq;
  l_squeue* txms;
  l_string log;
  l_filestream logfile;
  void* frbq;
  l_thrid id;
  int (*start)();
  void* block;
} l_thread;

l_extern l_thrkey L_thread_key;
l_extern_thread_local(l_thread* L_thread_ptr);

l_inline l_thread* l_thread_self() {
#if defined(l_thread_local_supported)
  return L_thread_ptr;
#else
  return (l_thread*)l_thrkey_get_data(&L_thread_key);
#endif
}

l_extern l_thread* l_thread_master();
l_extern int l_thread_start(l_thread* self, int (*start)());
l_extern int l_thread_join(l_thread* self);
l_extern void l_thread_flush_log(l_thread* self);
l_extern void l_thread_exit();
l_extern void l_process_exit();

/* test cases */

l_extern void l_core_test();
l_extern void l_string_test();
l_extern void l_luac_test();
l_specif void l_plat_test();
l_specif void l_plat_ionf_test();

#endif /* l_core_lib_h */
