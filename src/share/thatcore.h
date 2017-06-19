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
#undef l_thread_local_supported
#if defined(L_CLER_GCC)
#define l_thread_local(a) __thread a
#define l_thread_local_supported
#elif defined(L_CLER_MSC)
#define l_thread_local(a) __declspec(thread) a
#define l_thread_local_supported
#else
#define l_thread_local(a)
#endif

#define l_cast(type, a) ((type)(a))
#define l_str(s) ((l_rune*)(s))

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
#else
#define L_DEBUG_HERE(...) { ((void)0); }
#endif

typedef union {
  l_integer d;
  l_uinteger u;
  double f;
  const void* p;
} l_value;

#define ls(s) lp(s)
#define lserror(n) lp(strerror(n))
#define lstring(s) lp(l_string_cstr(s))

l_inline l_value lp(const void* p) {
  l_value a;
  a.p = p;
  return a;
}

l_inline l_value ld(l_integer d) {
  l_value a;
  a.d = d;
  return a;
}

l_inline l_value lu(l_uinteger u) {
  l_value a;
  a.u = u;
  return a;
}

l_inline l_value lf(double f) {
  l_value a;
  a.f = f;
  return a;
}

#define L_MKSTR(a) #a
#define L_X_MKSTR(a) L_MKSTR(a)
#define L_MKFLSTR __FILE__ " (" L_X_MKSTR(__LINE__) ") "

#define l_assert(e)               l_assert_func_impl((e), (#e), L_MKFLSTR) /* 0:assert */
#define l_loge_s(s)               l_logger_func_s("10[E] " L_MKFLSTR, (s)) /* 1:error */
#define l_loge_1(fmt, a)          l_logger_func_1("11[E] " L_MKFLSTR, (fmt), a)
#define l_loge_2(fmt, a, b)       l_logger_func_2("12[E] " L_MKFLSTR, (fmt), a, b)
#define l_loge_3(fmt, a, b, c)    l_logger_func_3("13[E] " L_MKFLSTR, (fmt), a, b, c)
#define l_loge_4(fmt, a, b, c, d) l_logger_func_4("14[E] " L_MKFLSTR, (fmt), a, b, c, d)
#define l_logw_s(s)               l_logger_func_s("20[W] " L_MKFLSTR, (s)) /* 2:warning */
#define l_logw_1(fmt, a)          l_logger_func_1("21[W] " L_MKFLSTR, (fmt), a);
#define l_logw_2(fmt, a, b)       l_logger_func_2("22[W] " L_MKFLSTR, (fmt), a, b);
#define l_logw_3(fmt, a, b, c)    l_logger_func_3("23[W] " L_MKFLSTR, (fmt), a, b, c);
#define l_logw_4(fmt, a, b, c, d) l_logger_func_4("24[W] " L_MKFLSTR, (fmt), a, b, c, d);
#define l_logm_s(s)               l_logger_func_s("30[A] " L_MKFLSTR, (s)) /* 3:main log */
#define l_logm_1(fmt, a)          l_logger_func_1("31[A] " L_MKFLSTR, (fmt), a);
#define l_logm_2(fmt, a, b)       l_logger_func_2("32[A] " L_MKFLSTR, (fmt), a, b);
#define l_logm_3(fmt, a, b, c)    l_logger_func_3("33[A] " L_MKFLSTR, (fmt), a, b, c);
#define l_logm_4(fmt, a, b, c, d) l_logger_func_4("34[A] " L_MKFLSTR, (fmt), a, b, c, d);
#define l_logd_s(s)               l_logger_func_s("40[D] " L_MKFLSTR, (s)) /* 4:debug log */
#define l_logd_1(fmt, a)          l_logger_func_1("41[D] " L_MKFLSTR, (fmt), a);
#define l_logd_2(fmt, a, b)       l_logger_func_2("42[D] " L_MKFLSTR, (fmt), a, b);
#define l_logd_3(fmt, a, b, c)    l_logger_func_3("43[D] " L_MKFLSTR, (fmt), a, b, c);
#define l_logd_4(fmt, a, b, c, d) l_logger_func_4("44[D] " L_MKFLSTR, (fmt), a, b, c, d);

l_extern void l_assert_func_impl(int pass, const void* expr, const void* fileline);
l_extern void l_logger_func_impl(const void* tag, const void* fmt, ...);
l_extern void l_set_log_level(int level);
l_extern int l_get_log_level();
l_extern void l_process_exit();

l_inline void l_logger_func_s(const void* tag, const void* s) {
  l_logger_func_impl(tag, s, 0);
}

l_inline void l_logger_func_1(const void* tag, const void* s, l_value a) {
  l_logger_func_impl(tag, s, a);
}

l_inline void l_logger_func_2(const void* tag, const void* s, l_value a, l_value b) {
  l_logger_func_impl(tag, s, a, b);
}

l_inline void l_logger_func_3(const void* tag, const void* s, l_value a, l_value b, l_value c) {
  l_logger_func_impl(tag, s, a, b, c);
}

l_inline void l_logger_func_4(const void* tag, const void* s, l_value a, l_value b, l_value c, l_value d) {
  l_logger_func_impl(tag, s, a, b, c, d);
}

typedef struct {
  const l_byte* start;
  const l_byte* end;
} l_strt;

#define l_empty_strt() ((l_strt){0,0})
#define l_literal_strt(s) l_strt_l("" s, (sizeof(s)/sizeof(char))-1)
#define l_strt_c(s) l_strt_l((s), strlen(l_cast(char*, (s))))
#define l_strt_e(s, e) l_strt_l((s), l_str(e) - l_str(s))
l_extern l_strt l_strt_l(const void* s, l_integer len);
l_extern int l_strt_contain(l_strt s, int ch);

#define l_strt_equal_c(s, str) l_strt_equal_l((s), (str), strlen((char*)(str)))
l_extern int l_strt_equal_l(l_strt s, const void* str, l_integer len);

#define l_zero_e(start, end) l_zero_l(start, l_str(end) - l_str(start))
l_extern void l_zero_l(void* start, l_integer len);

#define l_copy_e(from, end, to) l_copy_l(from, l_str(end) - l_str(from), (to))
l_extern void l_copy_l(const void* from, l_integer len, void* to);

l_extern void* l_raw_malloc(l_int size);
l_extern void* l_raw_calloc(l_int size);
l_extern void* l_raw_realloc(void* buffer, l_int oldsize, l_int newsize);
l_extern void l_raw_free(void* buffer);

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
  l_intptr* a; /* array of elements with pointer size */
  int (*less)(void* this_elem_is_less, void* than_this_one);
} l_mmheap;

/* min. heap - pass less function to less, max. heap - pass greater function to less */
l_extern void l_mmheap_init(l_mmheap* self, int (*less)(void*, void*), int initsize);
l_extern void l_mmheap_free(l_mmheap* self);
l_extern void l_mmheap_add(l_mmheap* self, void* elem);
l_extern void* l_mmheap_del(l_mmheap* self, l_umedit i);

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
l_specif l_date l_date_from_secs(l_integer utcsecs);
l_specif l_date l_date_from_time(l_time utc);

typedef struct {
  void* stream;
} l_filestream;

typedef struct {
  l_filestream f;
  l_rune* a;
  int capacity;
  int size;
} l_logger;

l_extern l_filestream l_open_read(const void* name);
l_extern l_filestream l_open_write(const void* name);
l_extern l_filestream l_open_append(const void* name);
l_extern l_filestream l_open_read_write(const void* name);
l_extern l_filestream l_open_write_unbuffered(const void* name);
l_extern l_filestream l_open_append_unbuffered(const void* name);
l_extern l_integer l_write_file(l_filestream* self, const void* s, l_integer len);
l_extern l_integer l_read_file(l_filestream* self, void* out, l_integer len);

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

typedef struct {
  l_integer fsize;
  l_integer ctm; /* creation, utc */
  l_integer atm; /* last access, utc */
  l_integer mtm; /* last modify, utc */
  l_integer gid;
  l_integer uid;
  l_integer mode;
  l_byte isfile;
  l_byte isdir;
  l_byte islink;
} l_fileattr;

typedef struct {
  void* stream;
} l_dirstream;

l_specif l_integer l_file_size(const void* name);
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
l_specif int l_condv_timedwait(l_condv* self, l_mutex* mutex, l_integer ns);
l_specif void l_condv_signal(l_condv* self);
l_specif void l_condv_broadcast(l_condv* self);

l_specif l_thrid l_raw_self_thread();
l_specif int l_raw_create_thread(l_thrid* thrid, void* (*start)(void*), void* para);
l_specif int l_raw_thread_join(l_thrid* thrid);
l_specif void l_raw_thread_exit();
l_specif void l_thread_sleep(l_integer us);

#define L_COMMON_BUFHEAD \
  l_squeue node; \
  l_int bsize;

l_extern void* l_thread_alloc_buffer(l_thread* thread, l_int sizeofelem);
l_extern void* l_thread_ensure_bfsize(l_thread* thread, l_smplnode* elem, l_int newsz);
l_extern void l_thread_free_buffer(l_thread* thread, l_smplnode* elem, void (*elemfree)(void*));

typedef struct lua_State lua_State;
typedef struct l_service l_service;

typedef struct l_thread {
  l_linknode node;
  l_umedit weight;
  l_ushort index;
  l_ushort flags;
  /* shared with master */
  l_mutex* elock;
  l_mutex* mutex;
  l_condv* condv;
  l_dqueue _workrxq;
  l_squeue msgrxq;
  /* thread own use */
  lua_State* L;
  l_dqueue _workq;
  l_squeue msgq;
  l_logger* log;
  l_freeq* frco;
  l_freeq* frbf;
  int (*start)();
  l_thrid id;
  void* block;
} l_thread;

typedef struct l_state {
  l_smplnode node;
  l_thread* belong;
  lua_State* co;
  int coref;
  int (*func)(struct l_state*);
  int (*kfunc)(struct l_state*);
  l_service* srvc;
} l_state;

l_extern l_thrkey llg_thread_key;
l_extern l_thrkey llg_logger_key;

l_thread_local(l_thread* llg_thread_ptr);
l_thread_local(l_logger* llg_logger_ptr);

l_inline l_thread* l_thread_self() {
#if defined(l_thread_local_supported)
  return llg_thread_ptr;
#else
  return (l_thread*)l_thrkey_get_data(&llg_thread_key);
#endif
}

l_inline l_thread* l_state_thread(l_state* s) {
  return s->belong;
}

l_inline l_logger* l_thread_logger() {
#if defined(l_thread_local_supported)
  return llg_logger_ptr;
#else
  return (l_thread*)l_thrkey_get_data(&llg_logger_key);
#endif
}

l_inline l_logger* l_state_logger(l_state* s) {
  return s->belong->log;
}

l_extern void l_thread_exit();
l_extern int l_thread_join(l_thread* self);
l_extern int l_thread_start(l_thread* self, int (*start)());

typedef struct {
  L_COMMON_BUFHEAD
  l_thread* belong;
  l_int limit;
  l_int size;
} l_buffer;

l_inline l_byte* l_buffer_gets(l_buffer* self) {
  return (l_byte*)(self + 1);
}

l_extern l_buffer* l_buffer_new(l_thread* thread, l_int initsize, l_int maxlimit);
l_extern void l_buffer_free(l_buffer* buffer);
l_extern int l_buffer_ensure_capacity(l_buffer** self, l_int size);
l_extern int l_buffer_ensure_remain(l_buffer** self, l_int remainsize);

typedef struct {
  l_buffer* b;
} l_string;

l_inline const l_rune* l_string_cstr(l_string* self) {
  return l_buffer_gets(self->b);
}

l_inline l_strt l_string_strt(l_string* self) {
  const l_rune* s = l_buffer_gets(self->b);
  return (l_strt){s, s + self->b->size};
}

l_extern l_string l_string_new(l_strt from);
l_extern l_string l_thread_string_new(l_thread* thread, l_strt from);
l_extern void l_string_free(l_string* self);
l_extern void l_string_clear(l_string* self);
l_extern void l_string_setstrt(l_string* self, l_strt s);
l_extern void l_string_setcstr(l_string* self, const void* s);
l_extern void l_string_setlstr(l_string* self, const void* s, l_integer len);
l_extern int l_string_is_empty(l_string* self);
l_extern int l_string_equal_c(l_string* self, const void* s);
l_extern int l_string_equal_l(l_string* self, const void* s, l_integer len);

typedef struct {
  l_umedit m; /* string match this rune */
  l_umedit e; /* string ended with this rune */
} l_runeinfo;

typedef struct { /* 4 * 256 * 2 = 2048 (2KB) */
  l_runeinfo a[256];
} l_runetable;

typedef struct {
  l_runetable* t; /* table array, 1 table contains 1 rune, the array size is up to the length of the string */
  int size; /* a string map can store strings up to 'maxnumofstr', the size is the length of the longest string */
  int maxnumofstr;
} l_stringmap;

l_extern const l_stringmap* l_string_space_map();
l_extern const l_stringmap* l_string_newline_map();
l_extern const l_stringmap* l_string_blank_map();

l_extern l_stringmap l_string_new_map(int maxstrlen, const l_rune** str, int numofstr, int casesensitive);
l_extern void l_string_set_map(l_stringmap* self, const l_rune** str, int numofstr, int casesensitive);
l_extern void l_string_free_map(l_stringmap* self);

l_extern const l_rune* l_string_match(const l_stringmap* map, const void* s, int len);
l_extern const l_rune* l_string_match_ex(const l_stringmap* map, const void* s, int len, int* strid, int* mlen);
l_extern const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, const void* s, int len);
l_extern const l_rune* l_string_match_repeat(const l_stringmap* map, const void* s, int len, int* lastmatchfailed);
l_extern const l_rune* l_string_match_until(const l_stringmap* map, const void* s, int len, int* n);
l_extern const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, const void* s, int len, int* n);
l_extern const l_rune* l_string_skip_space_and_match(const l_stringmap* map, const void* s, int len, int* strid, int* mlen);


l_extern void l_core_test();
l_extern void l_plat_test();
l_extern void l_string_test();

#endif /* l_core_lib_h */
