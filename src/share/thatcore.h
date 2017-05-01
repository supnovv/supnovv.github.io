#ifndef LIBC_THATCORE_H_
#define LIBC_THATCORE_H_
#include "ccprefix.h"
#include "autoconf.h"

#undef CORE_API
#define CORE_API extern

#undef CLUA_API
#if defined(CCLUASHARED)
  #if defined(__GNUC__)
    #define CLUA_API extern
  #else
  #if defined(WINDOWS_CORE_LIB) || defined(THATCORE_LIB)
    #define CLUA_API __declspec(dllexport)
  #else
    #define CLUA_API __declspec(dllimport)
  #endif
  #endif
#else
  #define CLUA_API extern
#endif

#define CC_BYTE_MAX ((byte)0xFF) /* 255 */
#define CC_INT8_MAX ((int8)0x7F) /* 127 */
#define CC_INT8_MIN ((int8)(-127-1)) /* 128 0x80 */
#define CC_USHT_MAX ((ushort)0xFFFF) /* 65535 */
#define CC_SSHT_MAX ((_short)0x7FFF) /* 32767 */
#define CC_SSHT_MIN ((_short)-32767-1) /* 32768 0x8000 */
#define CC_UMED_MAX ((umedit)0xFFFFFFFF) /* 4294967295 */
#define CC_SMED_MAX ((_medit)0x7FFFFFFF) /* 2147483647 */
#define CC_SMED_MIN ((_medit)-2147483647-1) /* 2147483648 0x80000000 */
#define CC_UINT_MAX ((uint)0xFFFFFFFFFFFFFFFF) /* 18446744073709551615 */
#define CC_SINT_MAX ((_int)0x7FFFFFFFFFFFFFFF) /* 9223372036854775807 */
#define CC_SINT_MIN ((_int)-9223372036854775807-1) /* 9223372036854775808 0x8000000000000000 */

struct ccheap {
  byte* start;
  byte* beyond;
};

struct ccfrom {
  const byte* start;
  const byte* beyond;
};

struct ccdest {
  byte* start;
  byte* beyond;
};

#define CCSTRING_SIZEOF 32
#define CCSTRING_STATIC_CHARS 30

struct ccstring {
  struct ccheap heap;
  _int len;
  byte a[CCSTRING_SIZEOF-sizeof(struct ccheap)-sizeof(_int)-1];
  byte flag; /* flag==0xFF ? heap-string : stack-string */
}; /* a sequence of bytes with zero terminated */ 

/** Time and date **
The 64-bit signed integer's biggest value is 9223372036854775807.
For seconds/milliseconds/microseconds/nanoseconds, it can
represent more than 291672107014/291672107/291672/291 years.
The 32-bit signed integer's biggest value is 2147483647.
For seconds/milliseconds/microseconds/nanoseconds, it can
represent more than 67-year/24-day/35-min/2-sec. */

struct cctime {
  _int sec;
  umedit nsec;
};

struct ccdate {
  umedit yearlow; /* 38-bit, can represent 274877906943 years */
  byte high;      /* high 6-bit are extra bits for year */
  byte ydaylow;   /* 1~366 */
  byte month;     /* 1~12 */
  byte day;       /* 1~31 */
  byte wday;      /* 0~6, 0 is sunday */
  byte hour;      /* 0~23 */
  byte min;       /* 0~59 */
  byte sec;       /* 0~61, 60 and 61 are the leap seconds */
  umedit nsec;    /* nanoseconds that less than 1 sec */
};

struct ccfileattr {
  _int fsize;
  _int ctime;
  _int atime;
  _int mtime;
  _int gid;
  _int uid;
  _int mode;
  bool isfile;
  bool isdir;
  bool islink;
}; /* creation, last access, last modify (UTC) */

/** Debugger and logger **/

#ifdef CCDEBUG
#define CC_DEBUG_ZONE(...) { __VA_ARGS__ } 
#else
#define CC_DEBUG_ZONE(...) { ((void)0); }
#endif

#define CCXMKSTR(a) #a
#define CCMKSTR(a) CCXMKSTR(a)
#define CCFILELINESTR __FILE__ " line " CCMKSTR(__LINE__)
#define CCLOGTAGSTR __FILE__ " (" CCMKSTR(__LINE__) ") "

#define ccassert(e) cc_assert_func((e), (#e), CCFILELINESTR)                        /* 0:assert */
#define ccloge(fmt, ...) cc_logger_func("1[E] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 1:error */
#define cclogw(fmt, ...) cc_logger_func("2[W] " CCLOGTAGSTR, (fmt), ##　__VA_ARGS__) /* 2:warning */
#define cclogi(fmt, ...) cc_logger_func("3[I] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 3:important */
#define cclogd(fmt, ...) cc_logger_func("4[D] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 4:debug */

CORE_API void cc_assert_func(bool pass, const char* expr, const char* fileline);
CORE_API _int cc_logger_func(const char* tag, const void* fmt, ...);
CORE_API void ccsetloglevel(_int loglevel);
CORE_API _int ccgetloglevel();
CORE_API void ccexit();

/** Memory operation **/

CORE_API void* ccrawalloc(_int size);
CORE_API void* ccrawrealloc(void* buffer, _int newsize);
CORE_API void ccrawfree(void* buffer);

CORE_API struct ccheap ccheap_alloc(_int size);
CORE_API struct ccheap ccheap_allocrawbuffer(_int size);
CORE_API struct ccheap ccheap_allocfrom(_int size, struct ccfrom from);
CORE_API void ccheap_realloc(struct ccheap* self, _int newsize);
CORE_API void ccheap_free(struct ccheap* self);

CORE_API _int cczero(void* p, _int bytes);
CORE_API _int cczerodest(struct ccdest dest);
CORE_API _int cccopy(struct ccfrom from, void* to);
CORE_API _int cccopyn(struct ccfrom from, _int n, void* to);
CORE_API _int cccopyr(struct ccfrom from, void* to);
CORE_API _int cccopyrn(struct ccfrom from, _int n, void* to);
CORE_API struct ccfrom ccfrom(const void* p, _int bytes);
CORE_API struct ccfrom ccfromcstr(const void* cstr);

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
CORE_API bool cclinknode_isempty(struct cclinknode* node);
CORE_API void cclinknode_insertafter(struct cclinknode* node, struct cclinknode* newnode);
CORE_API void ccsmplnode_init(struct ccsmplnode* node);
CORE_API bool ccsmplnode_isempty(struct ccsmplnode* node);
CORE_API void ccsmplnode_insertafter(struct ccsmplnode* node, struct ccsmplnode* newnode);
CORE_API void ccsqueue_init(struct ccsqueue* self);
CORE_API void ccsqueue_push(struct ccsqueue* self, struct ccsmplnode* newnode);
CORE_API bool ccsqueue_isempty(struct ccsqueue* self);
CORE_API void ccdqueue_init(struct ccdqueue* self);
CORE_API void ccdqueue_push(struct ccdqueue* self, struct cclinknode* newnode);
CORE_API bool ccdqueue_isempty(struct ccdqueue* self);
CORE_API struct cclinknode* cclinknode_remove(struct cclinknode* node);
CORE_API struct ccsmplnode* ccsmplnode_removenext(struct ccsmplnode* node);
CORE_API struct ccsmplnode* ccsqueue_pop(struct ccsqueue* self);
CORE_API struct cclinknode* ccdqueue_pop(struct ccdqueue* self);

/* Thread and synchronization */

struct ccmsghead {
  struct ccsmplnode node;
  umedit size;
  ushort type;
  ushort flag;
};

union cceight {
  _int ival;
  double dval;
  void* pval;
  byte b[8];
};

struct ccmutex {
  union cceight a[(CC_MUTEX_BYTES - 1) / sizeof(union cceight) + 1];
};

struct ccrwlock {
  union cceight a[(CC_RWLOCK_BYTES - 1) / sizeof(union cceight) + 1];
};

struct cccondv {
  union cceight a[(CC_CONDV_BYTES - 1) / sizeof(union cceight) + 1];
};

struct ccthread {
  union cceight a[(CC_THRID_BYTES - 1) / sizeof(union cceight) + 1];
};

struct ccthrkey {
  union cceight a[(CC_THKEY_BYTES - 1) / sizeof(union cceight) + 1];
};

CORE_API bool ccthrkey_init(struct ccthrkey* self);
CORE_API void ccthrkey_free(struct ccthrkey* self);
CORE_API bool ccthrkey_setdata(struct ccthrkey* self, const void* data);
CORE_API void* ccthrkey_getdata(struct ccthrkey* self);

struct ccstate;
struct ccionf;

struct ccglobal {
  struct ccstate* master;
  struct ccionf* ionf;
  struct ccmutex mutex;
};

struct ccstate {
  struct cclinknode node;
  struct ccglobal* g;
  void* lstate;
  struct ccthread thrid;
  umedit index, weight;
  struct ccmutex mutex;
  struct cccondv condv;
};

CORE_API int mainentry();
CORE_API _int ccthreadindex();
CORE_API void ccsetthreaddata(void* data);
CORE_API void* ccgetthreaddata();
CORE_API void ccsetcmdline(int argc, char** argv);
CORE_API struct ccfrom ccgetcmdline(struct ccfrom name);
CORE_API struct ccthread* ccthreadstart(void* (*start)(void*), void* para); /* platform-specific */
CORE_API struct ccthread* ccthreadself(); /* platform-specific */
CORE_API void ccthreadsleep(uint us); /* platform-specific */
CORE_API void ccthreadexit(); /* platform-specific */
CORE_API void* ccthreadjoin(struct ccthread* thread); /* platform-specific */
CORE_API struct ccmutex ccmutexcreate(); /* platform-specific */
CORE_API void ccmutexdestroy(struct ccmutex* self); /* platform-specific */
CORE_API bool ccmutexlock(struct ccmutex* self); /* platform-specific */
CORE_API bool ccmutextrylock(struct ccmutex* self); /* platform-specific */
CORE_API void ccmutexunlock(struct ccmutex* self); /* platform-specific */
CORE_API struct ccrwlock ccrwlockcreate(); /* platform-specific */
CORE_API void ccrwlockdestroy(struct ccrwlock* self); /* platform-specific */
CORE_API bool ccrwlockread(struct ccrwlock* self); /* platform-specific */
CORE_API bool ccrwlocktryread(struct ccrwlock* self); /* platform-specific */
CORE_API bool ccrwlockwrite(struct ccrwlock* self); /* platform-specific */
CORE_API bool ccrwlocktrywrite(struct ccrwlock* self); /* platform-specific */
CORE_API void ccrwlockunlock(struct ccrwlock* self); /* platform-specific */

/* Core test */

CORE_API void ccthattest();
CORE_API void ccplattest(); /* platfrom-specific */

#endif /* LIBC_THATCORE_H_ */
