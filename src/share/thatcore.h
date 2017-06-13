#ifndef l_core_lib_h
#define l_core_lib_h
#include "ccprefix.h"
#include "autoconf.h"

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

#define L_MKSTR(a) #a
#define L_X_MKSTR(a) L_MKSTR(a)
#define L_MKFLSTR __FILE__ " (" L_X_MKSTR(__LINE__) ") "

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

l_extern void* l_raw_alloc(l_integer size);
l_extern void* l_raw_alloc_and_zero(l_integer size);
l_extern void* l_raw_realloc(void* buffer, l_integer oldsize, l_integer newsize);
l_extern void l_raw_free(void* buffer);

l_extern l_integer l_zeroe(void* start, const void* end);
l_extern l_integer l_zerol(void* start, l_integer len);

typedef struct {
  const l_byte* start;
  const l_byte* end;
} l_from;

#define l_from_empty() l_cast(l_from, {0,0})
#define l_from_literal(s) l_froml("" s, (sizeof(s)/sizeof(char))-1)
l_extern l_from l_fromc(const void* s)
l_extern l_from l_froml(const void* s, int len);
l_extern l_from l_frome(const void* s, const void* e);

#if 0
typedef struct {
  l_byte* start;
  l_byte* end;
} l_dest;

l_extern l_dest l_heap_alloc(sright_int size);
l_extern l_dest l_heap_allocrawbuffer(sright_int size);
l_extern l_dest l_heap_allocfrom(sright_int size, l_from from);
l_extern void l_heap_relloc(l_heap* self, sright_int newsize);
l_extern void l_heap_free(l_heap* self);

l_extern l_integer l_copy(const l_byte* fromstart, const l_byte* frombeyond, l_byte* dest);
l_extern l_integer l_copyn(const l_byte* from, sright_int n, l_byte* dest);
l_extern l_integer l_copyfrom(l_from from, void* dest);
l_extern l_integer l_copyfromp(const l_from* from, void* dest);
l_extern l_integer l_copyfromtodest(l_from from, const l_dest* dest);
l_extern l_integer l_copyfromptodest(const l_from* from, const l_dest* dest);

l_extern l_integer l_rcopy(const l_byte* fromstart, const l_byte* frombeyond, l_byte* dest);
l_extern l_integer l_rcopyn(const l_byte* from, l_int n, l_byte* dest);
l_extern l_integer l_rcopyfrom(l_from from, void* dest);
l_extern l_integer l_rcopyfromp(const l_from* from, void* dest);
l_extern l_integer l_rcopyfromtodest(l_from from, const l_dest* dest);
l_extern l_integer l_rcopyfromptodest(const l_from* from, const l_dest* dest);

l_extern l_from l_fromp(const void* start, const void* beyond);
l_extern l_from l_fromn(const void* start, l_integer bytes);
l_extern l_from l_fromcstr(const void* cstr);
l_extern l_dest l_dest(void* start, l_integer bytes);
l_extern l_dest l_destrange(void* start, void* beyond);

l_extern l_from* l_setfrom(l_from* self, const void* start, l_integer bytes);
l_extern l_from* l_setfromcstr(l_from* self, const void* cstr);
l_extern l_from* l_setfromrange(l_from* self, const void* start, const void* beyond);
l_extern l_dest* l_setdest(l_dest* self, void* start, l_integer bytes);
l_extern l_dest* l_setdestrange(l_dest* self, void* start, void* beyond);
l_extern const l_dest* l_destheap(const l_heap* heap);

#endif

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

/** String **/

#define CCSTRING_SIZEOF 32
#define CCSTRING_STATIC_CHARS 30

typedef struct l_string {
  l_heap heap;
  sright_int len;
  uoctet_int a[CCSTRING_SIZEOF-sizeof(l_heap)-sizeof(sright_int)-1];
  uoctet_int flag; /* flag==0xFF ? heap-string : stack-string */
} l_string; /* a sequence of bytes with zero terminated */

l_extern const char* l_utos(uright_int a);
l_extern const char* l_itos(sright_int a);
l_extern l_string l_emptystr();
l_extern l_string l_strfromu(uright_int a);
l_extern l_string l_strfromuf(uright_int a, sright_int fmt);
l_extern l_string l_strfromi(sright_int a);
l_extern l_string l_strfromif(sright_int a, sright_int fmt);

l_extern l_string l_string_emptystr();
l_extern sright_int l_string_getlen(const l_string* self);
l_extern const char* l_string_getcstr(const l_string* self);
l_extern int l_string_equalcstr(const l_string* self, const void* cstr);

l_extern void l_string_free(l_string* self);
l_extern void l_string_setempty(l_string* self);
l_extern void l_string_setcstr(l_string* self, const void* cstr);

l_extern l_from l_string_getfrom(const l_string* s);
l_extern int l_string_contain(l_from s, nauty_char ch);
l_extern int l_string_containp(const l_from* s, nauty_char ch);

#if 0
CCINLINE nauty_char l_lower(nauty_char ch) {
  return (ch >= 'A' && ch <= 'Z') ? (ch + 32) : ch;
}

CCINLINE void l_lowerp(nauty_char* ch) {
  *ch = l_lower(*ch);
}

CCINLINE nauty_char l_upper(nauty_char ch) {
  return (ch >= 'a' && ch <= 'z') ? (ch - 32) : ch;
}

CCINLINE void l_upperp(nauty_char* ch) {
  *ch = l_upper(*ch);
}

CCINLINE void l_string_lower(nauty_char* s, int len) {
  while (len > 0) {
    l_lowerp(s + (--len));
  }
}

CCINLINE void l_string_upper(nauty_char* s, int len) {
  while (len > 0) {
    l_upperp(s + (--len));
  }
}

CCINLINE int l_isblank(int ch) {
  if (ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f') return 1;
  return 0;
}
#endif

/** List and queue **/

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
