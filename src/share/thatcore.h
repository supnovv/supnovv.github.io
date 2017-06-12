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

/** Memory operation **/

typedef struct ccfrom {
  const nauty_byte* start;
  const nauty_byte* beyond;
} ccfrom;

struct ccdest {
  nauty_byte* start;
  nauty_byte* beyond;
};

struct ccheap {
  nauty_byte* start;
  nauty_byte* beyond;
};

CORE_API struct ccheap ccheap_alloc(sright_int size);
CORE_API struct ccheap ccheap_allocrawbuffer(sright_int size);
CORE_API struct ccheap ccheap_allocfrom(sright_int size, struct ccfrom from);
CORE_API void ccheap_relloc(struct ccheap* self, sright_int newsize);
CORE_API void ccheap_free(struct ccheap* self);

CORE_API sright_int cccopy(const nauty_byte* fromstart, const nauty_byte* frombeyond, nauty_byte* dest);
CORE_API sright_int cccopyn(const nauty_byte* from, sright_int n, nauty_byte* dest);
CORE_API sright_int cccopyfrom(struct ccfrom from, void* dest);
CORE_API sright_int cccopyfromp(const struct ccfrom* from, void* dest);
CORE_API sright_int cccopyfromtodest(struct ccfrom from, const struct ccdest* dest);
CORE_API sright_int cccopyfromptodest(const struct ccfrom* from, const struct ccdest* dest);

CORE_API sright_int ccrcopy(const nauty_byte* fromstart, const nauty_byte* frombeyond, nauty_byte* dest);
CORE_API sright_int ccrcopyn(const nauty_byte* from, sright_int n, nauty_byte* dest);
CORE_API sright_int ccrcopyfrom(struct ccfrom from, void* dest);
CORE_API sright_int ccrcopyfromp(const struct ccfrom* from, void* dest);
CORE_API sright_int ccrcopyfromtodest(struct ccfrom from, const struct ccdest* dest);
CORE_API sright_int ccrcopyfromptodest(const struct ccfrom* from, const struct ccdest* dest);

CORE_API struct ccfrom ccfromp(const void* start, const void* beyond);
CORE_API struct ccfrom ccfromn(const void* start, sright_int bytes);
CORE_API struct ccfrom ccfromcstr(const void* cstr);
CORE_API struct ccdest ccdest(void* start, sright_int bytes);
CORE_API struct ccdest ccdestrange(void* start, void* beyond);

CORE_API struct ccfrom* ccsetfrom(struct ccfrom* self, const void* start, sright_int bytes);
CORE_API struct ccfrom* ccsetfromcstr(struct ccfrom* self, const void* cstr);
CORE_API struct ccfrom* ccsetfromrange(struct ccfrom* self, const void* start, const void* beyond);
CORE_API struct ccdest* ccsetdest(struct ccdest* self, void* start, sright_int bytes);
CORE_API struct ccdest* ccsetdestrange(struct ccdest* self, void* start, void* beyond);
CORE_API const struct ccdest* ccdestheap(const struct ccheap* heap);

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

typedef struct ccstring {
  struct ccheap heap;
  sright_int len;
  uoctet_int a[CCSTRING_SIZEOF-sizeof(struct ccheap)-sizeof(sright_int)-1];
  uoctet_int flag; /* flag==0xFF ? heap-string : stack-string */
} ccstring; /* a sequence of bytes with zero terminated */

CORE_API const char* ccutos(uright_int a);
CORE_API const char* ccitos(sright_int a);
CORE_API ccstring ccemptystr();
CORE_API ccstring ccstrfromu(uright_int a);
CORE_API ccstring ccstrfromuf(uright_int a, sright_int fmt);
CORE_API ccstring ccstrfromi(sright_int a);
CORE_API ccstring ccstrfromif(sright_int a, sright_int fmt);

CORE_API ccstring ccstring_emptystr();
CORE_API sright_int ccstring_getlen(const ccstring* self);
CORE_API const char* ccstring_getcstr(const ccstring* self);
CORE_API nauty_bool ccstring_equalcstr(const ccstring* self, const void* cstr);

CORE_API void ccstring_free(ccstring* self);
CORE_API void ccstring_setempty(ccstring* self);
CORE_API void ccstring_setcstr(ccstring* self, const void* cstr);

CORE_API ccfrom ccstring_getfrom(const ccstring* s);
CORE_API nauty_bool ccstring_contain(struct ccfrom s, nauty_char ch);
CORE_API nauty_bool ccstring_containp(const struct ccfrom* s, nauty_char ch);

#if 0
CCINLINE nauty_char cclower(nauty_char ch) {
  return (ch >= 'A' && ch <= 'Z') ? (ch + 32) : ch;
}

CCINLINE void cclowerp(nauty_char* ch) {
  *ch = cclower(*ch);
}

CCINLINE nauty_char ccupper(nauty_char ch) {
  return (ch >= 'a' && ch <= 'z') ? (ch - 32) : ch;
}

CCINLINE void ccupperp(nauty_char* ch) {
  *ch = ccupper(*ch);
}

CCINLINE void ccstring_lower(nauty_char* s, int len) {
  while (len > 0) {
    cclowerp(s + (--len));
  }
}

CCINLINE void ccstring_upper(nauty_char* s, int len) {
  while (len > 0) {
    ccupperp(s + (--len));
  }
}

CCINLINE int ccisblank(int ch) {
  if (ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f') return 1;
  return 0;
}
#endif

/** List and queue **/

typedef struct cclinknode {
  struct cclinknode* next;
  struct cclinknode* prev;
} cclinknode;

CORE_API void cclinknode_init(cclinknode* node);
CORE_API nauty_bool cclinknode_isempty(cclinknode* node);
CORE_API void cclinknode_insertafter(cclinknode* node, cclinknode* newnode);
CORE_API cclinknode* cclinknode_remove(cclinknode* node);

typedef struct ccsmplnode {
  struct ccsmplnode* next;
} ccsmplnode;

CORE_API void ccsmplnode_init(ccsmplnode* node);
CORE_API nauty_bool ccsmplnode_isempty(ccsmplnode* node);
CORE_API void ccsmplnode_insertafter(ccsmplnode* node, ccsmplnode* newnode);
CORE_API ccsmplnode* ccsmplnode_removenext(ccsmplnode* node);

typedef struct ccsqueue {
  struct ccsmplnode head;
  struct ccsmplnode* tail;
} ccsqueue;

CORE_API void ccsqueue_init(ccsqueue* self);
CORE_API void ccsqueue_push(ccsqueue* self, ccsmplnode* newnode);
CORE_API void ccsqueue_pushqueue(ccsqueue* self, ccsqueue* queue);
CORE_API nauty_bool ccsqueue_isempty(ccsqueue* self);
CORE_API ccsmplnode* ccsqueue_pop(ccsqueue* self);

typedef struct ccdqueue {
  struct cclinknode head;
} ccdqueue;

CORE_API void ccdqueue_init(ccdqueue* self);
CORE_API void ccdqueue_push(ccdqueue* self, cclinknode* newnode);
CORE_API void ccdqueue_pushqueue(ccdqueue* self, ccdqueue* queue);
CORE_API nauty_bool ccdqueue_isempty(ccdqueue* self);
CORE_API cclinknode* ccdqueue_pop(ccdqueue* self);

typedef struct ccpriorq {
  struct cclinknode node;
  /* elem with less number has higher priority, i.e., 0 is the highest */
  nauty_bool (*less)(void* elem_is_less_than, void* this_one);
} ccpriorq;

CORE_API void ccpriorq_init(ccpriorq* self, nauty_bool (*less)(void*, void*));
CORE_API void ccpriorq_push(ccpriorq* self, cclinknode* elem);
CORE_API void ccpriorq_remove(ccpriorq* self, cclinknode* elem);
CORE_API nauty_bool ccpriorq_isempty(ccpriorq* self);
CORE_API cclinknode* ccpriorq_pop(ccpriorq* self);

/** Userful structures **/

/**
 * heap - add/remove quick, search slow
 */

#define CCMMHEAP_MIN_SIZE (8)
struct ccmmheap {
  umedit_int size;
  umedit_int capacity;
  unsign_ptr* a; /* array of elements with pointer size */
  nauty_bool (*less)(void* this_elem_is_less, void* than_this_one);
};

/* min. heap - pass less function to less, max. heap - pass greater function to less */
CORE_API void ccmmheap_init(struct ccmmheap* self, nauty_bool (*less)(void*, void*), int initsize);
CORE_API void ccmmheap_free(struct ccmmheap* self);
CORE_API void ccmmheap_add(struct ccmmheap* self, void* elem);
CORE_API void* ccmmheap_del(struct ccmmheap* self, umedit_int i);

/**
 * hash table - add/remove/search quick, need re-hash when enlarge buffer size
 */

CORE_API nauty_bool ccisprime(umedit_int n);
CORE_API umedit_int ccmidprime(nauty_byte bits);

struct cchashslot {
  void* next;
};

struct cchashtable {
  nauty_byte slotbits;
  ushort_int offsetofnext;
  umedit_int nslot; /* prime number size not near 2^n */
  umedit_int nbucket;
  umedit_int (*getkey)(void*);
  struct cchashslot* slot;
};

CORE_API void cchashtable_init(struct cchashtable* self, nauty_byte sizebits, int offsetofnext, umedit_int (*getkey)(void*));
CORE_API void cchashtable_free(struct cchashtable* self);
CORE_API void cchashtable_foreach(struct cchashtable* self, void (*cb)(void*));
CORE_API void cchashtable_add(struct cchashtable* self, void* elem);
CORE_API void* cchashtable_find(struct cchashtable* self, umedit_int key);
CORE_API void* cchashtable_del(struct cchashtable* self, umedit_int key);

/* table size is enlarged auto */
struct ccbackhash {
  struct cchashtable* cur;
  struct cchashtable* old;
  struct cchashtable a;
  struct cchashtable b;
};

CORE_API void ccbackhash_init(struct ccbackhash* self, nauty_byte initsizebits, int offsetofnext, umedit_int (*getkey)(void*));
CORE_API void ccbackhash_free(struct ccbackhash* self);
CORE_API void ccbackhash_add(struct ccbackhash* self, void* elem);
CORE_API void* ccbackhash_find(struct ccbackhash* self, umedit_int key);
CORE_API void* ccbackhash_del(struct ccbackhash* self, umedit_int key);

/** Thread and synchronization **/

struct ccmutex {
  CCPLAT_IMPL_SIZE(CC_MUTEX_BYTES);
};

struct ccrwlock {
  CCPLAT_IMPL_SIZE(CC_RWLOCK_BYTES);
};

struct cccondv {
  CCPLAT_IMPL_SIZE(CC_CONDV_BYTES);
};

struct ccthrid {
  CCPLAT_IMPL_SIZE(CC_THRID_BYTES);
};

struct ccthrkey {
  CCPLAT_IMPL_SIZE(CC_THKEY_BYTES);
};

/**
 * thread-specific data key
 */

CORE_API nauty_bool ccthrkey_init(struct ccthrkey* self);
CORE_API void ccthrkey_free(struct ccthrkey* self);
CORE_API nauty_bool ccthrkey_setdata(struct ccthrkey* self, const void* data);
CORE_API void* ccthrkey_getdata(struct ccthrkey* self);

/**
 * mutex
 */

CORE_API nauty_bool ccmutex_init(struct ccmutex* self);
CORE_API void ccmutex_free(struct ccmutex* self);
CORE_API nauty_bool ccmutex_lock(struct ccmutex* self);
CORE_API void ccmutex_unlock(struct ccmutex* self);
CORE_API nauty_bool ccmutex_trylock(struct ccmutex* self);

/**
 * read/write lock
 */

CORE_API nauty_bool ccrwlock_init(struct ccrwlock* self);
CORE_API void ccrwlock_free(struct ccrwlock* self);
CORE_API nauty_bool ccrwlock_read(struct ccrwlock* self);
CORE_API nauty_bool ccrwlock_write(struct ccrwlock* self);
CORE_API nauty_bool ccrwlock_tryread(struct ccrwlock* self);
CORE_API nauty_bool ccrwlock_trywrite(struct ccrwlock* self);
CORE_API void ccrwlock_unlock(struct ccrwlock* self);

/**
 * condition variable
 */

CORE_API nauty_bool cccondv_init(struct cccondv* self);
CORE_API void cccondv_free(struct cccondv* self);
CORE_API nauty_bool cccondv_wait(struct cccondv* self, struct ccmutex* mutex);
CORE_API nauty_bool cccondv_timedwait(struct cccondv* self, struct ccmutex* mutex, sright_int ns);
CORE_API void cccondv_signal(struct cccondv* self);
CORE_API void cccondv_broadcast(struct cccondv* self);

/**
 * thread
 */

CORE_API struct ccthrid ccplat_selfthread();
CORE_API nauty_bool ccplat_createthread(struct ccthrid* thrid, void* (*start)(void*), void* para);
CORE_API void ccplat_threadsleep(uright_int us);
CORE_API void ccplat_threadexit();
CORE_API int ccplat_threadjoin(struct ccthrid* thrid);

typedef struct {
  const l_byte* start;
  const l_byte* end;
} l_from;

#define l_from_literal(s) l_from_lstring("" s, (sizeof(s)/sizeof(char))-1)
#define l_from_empty() l_cast(l_from, {0})

l_extern l_from l_from_cstring(const void* s)
l_extern l_from l_from_lstring(const void* s, int len);

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
