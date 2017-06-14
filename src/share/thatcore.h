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
#define l_str(s) l_cast(l_rune*, (s))

#define l_max_rdwr_size (0x7fff0000) /* 2147418112 */
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

#ifdef L_BUILD_DEBUG
#define L_DEBUG_HERE(...) { __VA_ARGS__ }
#else
#define L_DEBUG_HERE(...) { ((void)0); }
#endif

typedef struct {
  void* file;
  l_rune* a;
  int capacity;
  int size;
} l_logger;

typedef union {
  l_integer d;
  l_uinteger u;
  double f;
  const void* p;
} l_value;

#define ls(s) lp(s)

l_inline l_value lp(const void* p) {
  l_value a;
  a.p = p;
  reutrn a;
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
l_extern void l_exit();

typedef struct {
  const l_byte* start;
  const l_byte* end;
} l_strt;

#define l_strt_empty() l_cast(l_strt, {0,0})
#define l_strt_literal(s) l_strt_l("" s, (sizeof(s)/sizeof(char))-1)
#define l_strt l_strt_c(s) l_strt_l((s), strlen(l_cast(char*, (s))))
#define l_strt l_strt_e(s, e) l_strt_l((s), l_str(e) - l_str(s))
l_extern l_strt l_strt_l(const void* s, l_integer len);

#define l_zero_e(void* start, const void* end) l_zero_l(start, l_str(end) - l_str(start))
l_extern void l_zero_l(void* start, l_integer len);

#define l_copy_e(from, end, to) l_copy_l(from, l_str(end) - l_str(from), (to))
l_extern void l_copy_l(const void* from, l_integer len, void* to);

l_extern void* l_raw_malloc(l_integer size);
l_extern void* l_raw_calloc(l_integer size);
l_extern void* l_raw_realloc(void* buffer, l_integer oldsize, l_integer newsize);
l_extern void l_raw_free(void* buffer);

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
} l_dir_stream;

l_extern l_time l_system_time();
l_extern l_time l_monotonic_time();
l_extern l_date l_system_date();
l_extern l_integer l_get_file_size(const void* name, int namelen);
l_extern l_fileattr l_get_file_attr(const void* name, int namelen);


typedef struct l_linknode {
  struct l_linknode* next;
  struct l_linknode* prev;
} l_linknode;

l_extern void l_linknode_init(l_linknode* node);
l_extern int l_linknode_isempty(l_linknode* node);
l_extern void l_linknode_insertafter(l_linknode* node, l_linknode* newnode);
l_extern l_linknode* l_linknode_remove(l_linknode* node);

typedef struct l_smplnode {
  struct l_smplnode* next;
} l_smplnode;

l_extern void l_smplnode_init(l_smplnode* node);
l_extern int l_smplnode_isempty(l_smplnode* node);
l_extern void l_smplnode_insertafter(l_smplnode* node, l_smplnode* newnode);
l_extern l_smplnode* l_smplnode_removenext(l_smplnode* node);

typedef struct l_squeue {
  l_smplnode head;
  l_smplnode* tail;
} l_squeue;

l_extern void l_squeue_init(l_squeue* self);
l_extern void l_squeue_push(l_squeue* self, l_smplnode* newnode);
l_extern void l_squeue_pushqueue(l_squeue* self, l_squeue* queue);
l_extern int l_squeue_isempty(l_squeue* self);
l_extern l_smplnode* l_squeue_pop(l_squeue* self);

typedef struct l_dqueue {
  l_linknode head;
} l_dqueue;

l_extern void l_dqueue_init(l_dqueue* self);
l_extern void l_dqueue_push(l_dqueue* self, l_linknode* newnode);
l_extern void l_dqueue_pushqueue(l_dqueue* self, l_dqueue* queue);
l_extern int l_dqueue_isempty(l_dqueue* self);
l_extern l_linknode* l_dqueue_pop(l_dqueue* self);

typedef struct l_priorq {
  l_linknode node;
  /* elem with less number has higher priority, i.e., 0 is the highest */
  int (*less)(void* elem_is_less_than, void* this_one);
} l_priorq;

l_extern void l_priorq_init(l_priorq* self, int (*less)(void*, void*));
l_extern void l_priorq_push(l_priorq* self, l_linknode* elem);
l_extern void l_priorq_remove(l_priorq* self, l_linknode* elem);
l_extern int l_priorq_isempty(l_priorq* self);
l_extern l_linknode* l_priorq_pop(l_priorq* self);

/** Userful structures **/

/**
 * heap - add/remove quick, search slow
 */

#define CCMMHEAP_MIN_SIZE (8)
struct l_mmheap {
  umedit_int size;
  umedit_int capacity;
  unsign_ptr* a; /* array of elements with pointer size */
  int (*less)(void* this_elem_is_less, void* than_this_one);
};

/* min. heap - pass less function to less, max. heap - pass greater function to less */
l_extern void l_mmheap_init(l_mmheap* self, int (*less)(void*, void*), int initsize);
l_extern void l_mmheap_free(l_mmheap* self);
l_extern void l_mmheap_add(l_mmheap* self, void* elem);
l_extern void* l_mmheap_del(l_mmheap* self, umedit_int i);

/**
 * hash table - add/remove/search quick, need re-hash when enlarge buffer size
 */

l_extern int l_isprime(umedit_int n);
l_extern umedit_int l_midprime(nauty_byte bits);

struct l_hashslot {
  void* next;
};

struct l_hashtable {
  nauty_byte slotbits;
  ushort_int offsetofnext;
  umedit_int nslot; /* prime number size not near 2^n */
  umedit_int nbucket;
  umedit_int (*getkey)(void*);
  l_hashslot* slot;
};

l_extern void l_hashtable_init(l_hashtable* self, nauty_byte sizebits, int offsetofnext, umedit_int (*getkey)(void*));
l_extern void l_hashtable_free(l_hashtable* self);
l_extern void l_hashtable_foreach(l_hashtable* self, void (*cb)(void*));
l_extern void l_hashtable_add(l_hashtable* self, void* elem);
l_extern void* l_hashtable_find(l_hashtable* self, umedit_int key);
l_extern void* l_hashtable_del(l_hashtable* self, umedit_int key);

/* table size is enlarged auto */
struct l_backhash {
  l_hashtable* cur;
  l_hashtable* old;
  l_hashtable a;
  l_hashtable b;
};

l_extern void l_backhash_init(l_backhash* self, nauty_byte initsizebits, int offsetofnext, umedit_int (*getkey)(void*));
l_extern void l_backhash_free(l_backhash* self);
l_extern void l_backhash_add(l_backhash* self, void* elem);
l_extern void* l_backhash_find(l_backhash* self, umedit_int key);
l_extern void* l_backhash_del(l_backhash* self, umedit_int key);

/** Thread and synchronization **/

struct l_mutex {
  CCPLAT_IMPL_SIZE(CC_MUTEX_BYTES);
};

struct l_rwlock {
  CCPLAT_IMPL_SIZE(CC_RWLOCK_BYTES);
};

struct l_condv {
  CCPLAT_IMPL_SIZE(CC_CONDV_BYTES);
};

struct l_thrid {
  CCPLAT_IMPL_SIZE(CC_THRID_BYTES);
};

struct l_thrkey {
  CCPLAT_IMPL_SIZE(CC_THKEY_BYTES);
};

/**
 * thread-specific data key
 */

l_extern int l_thrkey_init(l_thrkey* self);
l_extern void l_thrkey_free(l_thrkey* self);
l_extern int l_thrkey_setdata(l_thrkey* self, const void* data);
l_extern void* l_thrkey_getdata(l_thrkey* self);

/**
 * mutex
 */

l_extern int l_mutex_init(l_mutex* self);
l_extern void l_mutex_free(l_mutex* self);
l_extern int l_mutex_lock(l_mutex* self);
l_extern void l_mutex_unlock(l_mutex* self);
l_extern int l_mutex_trylock(l_mutex* self);

/**
 * read/write lock
 */

l_extern int l_rwlock_init(l_rwlock* self);
l_extern void l_rwlock_free(l_rwlock* self);
l_extern int l_rwlock_read(l_rwlock* self);
l_extern int l_rwlock_write(l_rwlock* self);
l_extern int l_rwlock_tryread(l_rwlock* self);
l_extern int l_rwlock_trywrite(l_rwlock* self);
l_extern void l_rwlock_unlock(l_rwlock* self);

/**
 * condition variable
 */

l_extern int l_condv_init(l_condv* self);
l_extern void l_condv_free(l_condv* self);
l_extern int l_condv_wait(l_condv* self, l_mutex* mutex);
l_extern int l_condv_timedwait(l_condv* self, l_mutex* mutex, sright_int ns);
l_extern void l_condv_signal(l_condv* self);
l_extern void l_condv_broadcast(l_condv* self);

/**
 * thread
 */

l_extern l_thrid l_plat_selfthread();
l_extern int l_plat_createthread(l_thrid* thrid, void* (*start)(void*), void* para);
l_extern void l_plat_threadsleep(uright_int us);
l_extern void l_plat_threadexit();
l_extern int l_plat_threadjoin(l_thrid* thrid);

#define l_byte ccnauty_byte
#define l_rune ccnauty_byte
#define l_sbyte ccoctet_int
#define l_short ccshort_int
#define l_medit ccmedit_int
#define l_integer ccnauty_int
#define l_ushort ccshort_uint
#define l_umedit ccmedit_uint
#define l_uinteger ccnauty_int
#define l_intptr ccnauty_iptr
#define l_uintptr ccnauty_uptr
#define l_handle cchandle_int

l_extern void l_core_test();
l_extern void l_plat_test();

#endif /* l_core_lib_h */
