#ifndef LIBC_THATCORE_H_
#define LIBC_THATCORE_H_
#include "ccprefix.h"
#include "autoconf.h"

#undef CORE_API
#undef CLUA_API
#define CORE_API extern
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
#define CC_SSHT_MAX ((sshort)0x7FFF) /* 32767 */
#define CC_SSHT_MIN ((sshort)-32767-1) /* 32768 0x8000 */
#define CC_UMED_MAX ((umedit)0xFFFFFFFF) /* 4294967295 */
#define CC_SMED_MAX ((smedit)0x7FFFFFFF) /* 2147483647 */
#define CC_SMED_MIN ((smedit)-2147483647-1) /* 2147483648 0x80000000 */
#define CC_UINT_MAX ((uint)0xFFFFFFFFFFFFFFFF) /* 18446744073709551615 */
#define CC_SINT_MAX ((sint)0x7FFFFFFFFFFFFFFF) /* 9223372036854775807 */
#define CC_SINT_MIN ((sint)-9223372036854775807-1) /* 9223372036854775808 0x8000000000000000 */

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

struct ccstring {
  struct ccheap heap;
  sint len;
  byte a[sizeof(struct string)-sizeof(struct ccheap)-sizeof(sint)-1];
  byte flag; /* flag==0xFF ? heap-string : stack-string */
}; /* a sequence of bytes with zero terminated */ 

#define CCSTRING_STATIC_CHARS 30

/** Time and date **
The 64-bit signed integer's biggest value is 9223372036854775807.
For seconds/milliseconds/microseconds/nanoseconds, it can
represent more than 291672107014/291672107/291672/291 years.
The 32-bit signed integer's biggest value is 2147483647.
For seconds/milliseconds/microseconds/nanoseconds, it can
represent more than 67-year/24-day/35-min/2-sec. */

struct cctime {
  sint sec;
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

struct ccattr {
  sint fsize;
  sint ctime;
  sint atime;
  sint mtime;
  sint gid;
  sint uid;
  sint mode;
  bool isfile;
  bool isdir;
  bool islink;
}; /* creation, last access, last modify (UTC) */

struct ccmutex {
  union cctypeunit a[CCMUTEX_NUNIT];
};

struct ccrwlock {
  union cctypeunit a[CCRWLOCK_NUNIT];
};

struct cccondv {
  union cctypeunit a[CCCONDV_NUNIT];
};

/** Debugger and logger **/

#ifdef CCDEBUG
#define ccdebug(...) { __VA_ARGS__ } 
#else
#define ccdebug(...) { ((void)0) }
#endif

#define CCXMKSTR(a) #a
#define CCMKSTR(a) CCXMKSTR(a)
#define CCFILELINESTR __FILE__ " line " CCMKSTR(__LINE__)
#define CCLOGTAGSTR __FILE__ " (" CCMKSTR(__LINE__) ") "

#define ccassert(e) ccxassert((e), (#e), CCFILELINESTR)                        /* 0:assert */
#define ccloge(fmt, ...) ccxlogger("1[E] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 1:error */
#define cclogw(fmt, ...) ccxlogger("2[W] " CCLOGTAGSTR, (fmt), ##　__VA_ARGS__) /* 2:warning */
#define cclogi(fmt, ...) ccxlogger("3[I] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 3:important */
#define cclogd(fmt, ...) ccxlogger("4[D] " CCLOGTAGSTR, (fmt), ## __VA_ARGS__) /* 4:debug */

CORE_API void ccxassert(bool pass, const char* expr, const char* fileline);
CORE_API void ccxlogger(const char* tag, const void* fmt, ...);
CORE_API void ccsetlevel(sint loglevel);
CORE_API sint ccloglevel();
CORE_API void ccexit();

/** Memory operation **/

CORE_API struct ccheap ccalloc(sint size);
CORE_API struct ccheap ccallocs(sint size, struct ccfrom s);
CORE_API bool ccrelloc(struct ccheap* self, uint size);
CORE_API void ccfree(struct ccheap* self);
CORE_API void cczero(void* p, uint bytes);
CORE_API void cczerob(void* start, const void* beyond);
CORE_API void cccopy(struct ccfrom from, void* to);
CORE_API void cccopyn(struct ccfrom from, uint n, void* to);
CORE_API void cccopyr(struct ccfrom from, void* to);
CORE_API void cccopyrn(struct ccfrom from, uint n, void* to);
CORE_API struct ccfrom ccfrom(const void* p, uint n);
CORE_API struct ccfrom ccfroms(const void* s);
CORE_API struct ccfrom ccnext(struct ccfrom from, uint n);
CORE_API struct ccdest ccdest(void* p, uint n);

/** List and queue **/

struct cclistnode {
  struct cclistnode* next;
  struct cclistnode* prev;
};

struct ccsmplnode {
  struct ccsmplnode* next;
};

struct ccdqueue {
  struct cclistnode head;
};

struct ccsqueue {
  struct ccsmplnode head;
  struct ccsmplnode* tail;
};

CORE_API void cclistinit(struct cclistnode* node);
CORE_API bool cclistvoid(struct cclistnode* node);
CORE_API void cclistadd(struct cclistnode* node, struct cclistnode* newnode);
CORE_API struct cclistnode* cclistrem(struct cclistnode* node);
CORE_API void ccslistinit(struct ccsmplnode* node);
CORE_API bool ccslistvoid(struct ccsmplnode* node);
CORE_API void ccslistadd(struct ccsmplnode* node, struct ccsmplnode* newnode);
CORE_API struct ccsmplnode* ccslistrmn(struct ccsmplnode* node);
CORE_API void ccsqueueinit(struct ccsqueue* queue);
CORE_API void ccsqueueadd(struct ccsqueue* queue, struct ccsmplnode* newnode);
CORE_API bool ccsqueuevoid(struct ccsqueue* queue);
CORE_API struct ccsmplnode* ccsqueuerem(struct ccsqueue* queue);
CORE_API void ccdqueueinit(struct ccdqueue* self);
CORE_API void ccdqueueadd(struct ccdqueue* self, struct cclistnode* newnode);
CORE_API bool ccdqueuevoid(struct ccdqueue* self);
CORE_API struct cclistnode* ccdqueuerem(struct ccsqueue* self);

/* Thread and synchronization */

struct ccmsghead {
  struct ccsmplnode node;
  umedit size;
  ushort type;
  ushort flag;
};

struct cctlskey {
  union cctypeunit a[CCTLSKEY_NUNIT];
};

CORE_API bool cctlskeyinit(struct cctlskey* self);
CORE_API void cctlskeyfree(struct cctlskey* self);
CORE_API bool cctlskeyset(struct cctlskey* self, const void* data);
CORE_API void* cctlskeyget(struct cctlskey* self);

struct ccstate;

struct ccglobal {
  struct ccstate* master;
  struct ccepoll epoll;
  struct ccmutex mutex;
  umedit indexseed;
};

struct ccstate {
  struct cclinknode node;
  struct ccglobal* g;
  void* lstate;
  struct ccthread thread;
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
