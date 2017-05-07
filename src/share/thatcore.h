#ifndef CCLIB_THATCORE_H_
#define CCLIB_THATCORE_H_
#include "ccprefix.h"
#include "autoconf.h"

#undef CORE_API
#define CORE_API extern

#undef CLUA_API
#if defined(CCLUASHARED)
  #if defined(__GNUC__)
    #define CLUA_API extern
  #else
  #if defined(CCLIB_CORE_WINOS) || defined(CCLIB_THATCORE)
    #define CLUA_API __declspec(dllexport)
  #else
    #define CLUA_API __declspec(dllimport)
  #endif
  #endif
#else
  #define CLUA_API extern
#endif

#define CC_UBYT_MAX ((uoctet_int)0xFF) /* 255 */
#define CC_SBYT_MAX ((soctet_int)0x7F) /* 127 */
#define CC_SBYT_MIN ((soctet_int)(-127-1)) /* 128 0x80 */
#define CC_USHT_MAX ((ushort_int)0xFFFF) /* 65535 */
#define CC_SSHT_MAX ((sshort_int)0x7FFF) /* 32767 */
#define CC_SSHT_MIN ((sshort_int)-32767-1) /* 32768 0x8000 */
#define CC_UMED_MAX ((umedit_int)0xFFFFFFFF) /* 4294967295 */
#define CC_SMED_MAX ((smedit_int)0x7FFFFFFF) /* 2147483647 */
#define CC_SMED_MIN ((smedit_int)-2147483647-1) /* 2147483648 0x80000000 */
#define CC_UINT_MAX ((uright_int)0xFFFFFFFFFFFFFFFF) /* 18446744073709551615 */
#define CC_SINT_MAX ((sright_int)0x7FFFFFFFFFFFFFFF) /* 9223372036854775807 */
#define CC_SINT_MIN ((sright_int)-9223372036854775807-1) /* 9223372036854775808 0x8000000000000000 */

struct ccfrom {
  const nauty_byte* start;
  const nauty_byte* beyond;
};

struct ccdest {
  nauty_byte* start;
  nauty_byte* beyond;
};

struct ccheap {
  nauty_byte* start;
  nauty_byte* beyond;
};

#define CCSTRING_SIZEOF 32
#define CCSTRING_STATIC_CHARS 30

struct ccstring {
  struct ccheap heap;
  sright_int len;
  uoctet_int a[CCSTRING_SIZEOF-sizeof(struct ccheap)-sizeof(sright_int)-1];
  uoctet_int flag; /* flag==0xFF ? heap-string : stack-string */
}; /* a sequence of bytes with zero terminated */ 

/** Time and date **
The 64-bit signed integer's biggest value is 9223372036854775807.
For seconds/milliseconds/microseconds/nanoseconds, it can
represent more than 291672107014/291672107/291672/291 years.
The 32-bit signed integer's biggest value is 2147483647.
For seconds/milliseconds/microseconds/nanoseconds, it can
represent more than 67-year/24-day/35-min/2-sec. */

struct cctime {
  sright_int sec;
  umedit_int nsec;
};

struct ccdate {
  umedit_int yearlow; /* 38-bit, can represent 274877906943 years */
  uoctet_int high;      /* high 6-bit are extra bits for year */
  uoctet_int ydaylow;   /* 1~366 */
  uoctet_int month;     /* 1~12 */
  uoctet_int day;       /* 1~31 */
  uoctet_int wday;      /* 0~6, 0 is sunday */
  uoctet_int hour;      /* 0~23 */
  uoctet_int min;       /* 0~59 */
  uoctet_int sec;       /* 0~61, 60 and 61 are the leap seconds */
  umedit_int nsec;    /* nanoseconds that less than 1 sec */
};

struct ccfileattr {
  sright_int fsize;
  sright_int ctime;
  sright_int atime;
  sright_int mtime;
  sright_int gid;
  sright_int uid;
  sright_int mode;
  nauty_bool isfile;
  nauty_bool isdir;
  nauty_bool islink;
}; /* creation, last access, last modify (UTC) */

struct ccdirstream {
  void* stream;
};

CORE_API struct cctime ccsystime();
CORE_API struct cctime ccinctime();
CORE_API struct ccdate ccgetdate();
CORE_API sright_int ccfilesize(struct ccfrom name);
CORE_API struct ccfileattr ccgetfileattr(struct ccfrom name);


/** Debugger and logger **/

#ifdef CCDEBUG
#define CC_DEBUG_ZONE(...) { __VA_ARGS__ } 
#else
#define CC_DEBUG_ZONE(...) { ((void)0); }
#endif

#define CCXMKSTR(a) #a
#define CCMKSTR(a) CCXMKSTR(a)
#define CCFILELINESTR __FILE__ " (" CCMKSTR(__LINE__) ") "
#define CCLOGTAGSTR __FILE__ " (" CCMKSTR(__LINE__) ") "

#define ccassert(e) cc_assert_func((e), (#e), CCFILELINESTR)                        /* 0:assert */
#define ccloge(fmt, ...) cc_logger_func("1[E] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 1:error */
#define cclogw(fmt, ...) cc_logger_func("2[W] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 2:warning */
#define cclogi(fmt, ...) cc_logger_func("3[I] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 3:important */
#define cclogd(fmt, ...) cc_logger_func("4[D] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 4:debug */

CORE_API void cc_assert_func(nauty_bool pass, const char* expr, const char* fileline);
CORE_API sright_int cc_logger_func(const char* tag, const void* fmt, ...);
CORE_API void ccsetloglevel(sright_int loglevel);
CORE_API sright_int ccgetloglevel();
CORE_API void ccexit();

/** Memory operation **/

CORE_API void* ccrawalloc(sright_int size);
CORE_API void* ccrawrelloc(void* buffer, sright_int size);
CORE_API void ccrawfree(void* buffer);

CORE_API struct ccheap ccheap_alloc(sright_int size);
CORE_API struct ccheap ccheap_allocrawbuffer(sright_int size);
CORE_API struct ccheap ccheap_allocfrom(sright_int size, struct ccfrom from);
CORE_API void ccheap_relloc(struct ccheap* self, sright_int newsize);
CORE_API void ccheap_free(struct ccheap* self);

CORE_API sright_int cczero(void* start, const void* beyond);
CORE_API sright_int cczeron(void* start, sright_int bytes);

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

CORE_API struct ccfrom ccfrom(const void* start, const void* beyond);
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

/** String **/

CORE_API const char* ccutos(uright_int a);
CORE_API const char* ccitos(sright_int a);
CORE_API struct ccstring ccemptystr();
CORE_API struct ccstring ccstrfromu(uright_int a);
CORE_API struct ccstring ccstrfromuf(uright_int a, sright_int fmt);
CORE_API struct ccstring ccstrfromi(sright_int a);
CORE_API struct ccstring ccstrfromif(sright_int a, sright_int fmt);

CORE_API struct ccstring ccstring_emptystr();
CORE_API sright_int ccstring_getlen(const struct ccstring* self);
CORE_API const char* ccstring_getcstr(const struct ccstring* self);
CORE_API nauty_bool ccstring_equalcstr(const struct ccstring* self, const void* cstr);

CORE_API void ccstring_free(struct ccstring* self);
CORE_API void ccstring_setempty(struct ccstring* self);
CORE_API void ccstring_setcstr(struct ccstring* self, const void* cstr);

CORE_API nauty_bool ccstring_contain(struct ccfrom s, nauty_char ch);
CORE_API nauty_bool ccstring_containp(const struct ccfrom* s, nauty_char ch);


/** List and queue **/

struct cclinknode {
  struct cclinknode* next;
  struct cclinknode* prev;
};

struct ccsmplnode {
  struct ccsmplnode* next;
};

struct ccdqueue {
  struct cclinknode head;
};

struct ccsqueue {
  struct ccsmplnode head;
  struct ccsmplnode* tail;
};

CORE_API void cclinknode_init(struct cclinknode* node);
CORE_API nauty_bool cclinknode_isempty(struct cclinknode* node);
CORE_API void cclinknode_insertafter(struct cclinknode* node, struct cclinknode* newnode);
CORE_API void ccsmplnode_init(struct ccsmplnode* node);
CORE_API nauty_bool ccsmplnode_isempty(struct ccsmplnode* node);
CORE_API void ccsmplnode_insertafter(struct ccsmplnode* node, struct ccsmplnode* newnode);
CORE_API void ccsqueue_init(struct ccsqueue* self);
CORE_API void ccsqueue_push(struct ccsqueue* self, struct ccsmplnode* newnode);
CORE_API void ccsqueue_pushqueue(struct ccsqueue* self, struct ccsqueue* queue);
CORE_API nauty_bool ccsqueue_isempty(struct ccsqueue* self);
CORE_API void ccdqueue_init(struct ccdqueue* self);
CORE_API void ccdqueue_push(struct ccdqueue* self, struct cclinknode* newnode);
CORE_API void ccdqueue_pushqueue(struct ccdqueue* self, struct ccdqueue* queue);
CORE_API nauty_bool ccdqueue_isempty(struct ccdqueue* self);
CORE_API struct cclinknode* cclinknode_remove(struct cclinknode* node);
CORE_API struct ccsmplnode* ccsmplnode_removenext(struct ccsmplnode* node);
CORE_API struct ccsmplnode* ccsqueue_pop(struct ccsqueue* self);
CORE_API struct cclinknode* ccdqueue_pop(struct ccdqueue* self);

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
CORE_API nauty_bool cccondv_timedwait(struct cccondv* self, struct ccmutex* mutex, struct cctime time);
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

#define CCRUNSTATUS_SLEEP 0x01
#define CCRUNSTATUS_RUNNING 0x02
#define CCRUNSTATUS_WAKEUP_TRIGGERED 0x04

struct ccthread {
  /* access by master only */
  struct cclinknode node;
  umedit_int weight;
  /* shared with master */
  umedit_int runstatus;
  struct ccdqueue workrxq;
  struct ccmutex mutex;
  struct cccondv condv;
  /* free access */
  umedit_int index;
  struct ccthrid id;
  /* thread own use */
  struct lua_State* L;
  int (*start)();
  struct ccdqueue workq;
  struct ccsqueue msgq;
  struct ccsqueue freeco;
  struct ccstring defstr;
};

CORE_API struct ccthread* ccthread_getself();
CORE_API struct ccthread* ccthread_getmaster();
CORE_API void ccthread_init(struct ccthread* self);
CORE_API void ccthread_free(struct ccthread* self);
CORE_API nauty_bool ccthread_start(struct ccthread* self, int (*start)());
CORE_API void ccthread_sleep(uright_int us);
CORE_API void ccthread_exit();
CORE_API int ccthread_join(struct ccthread* self);

/**
 * message
 */

#define CCMSGTYPE_REMOTE 0x01
#define CCMSGTYPE_MASTER 0x02

struct ccmsghead {
  struct ccsmplnode node;
  void* extra;
  umedit_int svid;
  umedit_int data;
};

CORE_API void ccservice_sendmsg(struct ccmsghead* msg);

/**
 * service
 */

#define CCSVFLAG_DONE 0x01
#define CCSVTYPE_REGULARC 0x01
#define CCSVTYPE_CSERVICE 0x02
#define CCSVTYPE_LSERVICE 0x03

struct ccservice {
  struct cclinknode node;
  struct ccthread* thrd; /* modify by master only */
  struct ccsqueue rxmq;
  ushort_int oflag; /* shared with master */
  ushort_int iflag; /* internal use */
  umedit_int svid;
  void* udata;
  struct ccluaco* co;
  union {
  int (*func)(void* udata, void* msg);
  int (*cofunc)(struct ccluaco*);
  } u;
};



/** Core test **/

CORE_API void ccthattest();
CORE_API void ccplattest();

#endif /* CCLIB_THATCORE_H_ */
