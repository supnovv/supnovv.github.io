#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include "thatcore.h"

/** The software clock, HZ, and jiffies **
The accuracy of various time related system calls is limited
by the resolution of the software clock, a clock maintained
by the kernel which measures time in jiffies. The size of a
jiffy is determined by the value of the kernel constant HZ.
The value of HZ varies across kernel versions and hardware
platforms. On i386 the situation is as follows: on kernels up
to and including 2.4.x, HZ was 100, giving a jiffy value of
0.01 seconds; starting with 2.6.0, HZ was raised to 1000,
giving a jiffy of 0.001 seconds. Since kernel 2.6.13, the HZ
value is a kernel configuration parameter and can be 100, 250
(the default) or 1000. Since kernel 2.6.20, a further frequency
is available: 300, a number that divides evently for the common
video frame rates (PAL, 25HZ; NTSC, 30HZ).
 ** High-resolution timers **
Before Linux 2.6.21, the accuraccy of timer and sleep system
calls was limited by the size of the jiffy.
Since Linux 2.6.21, Linux supports high-resolution timers (HRTs),
optionally configuration via CONFIG_HIGH_RES_TIMERS. On a system
that supports HRTs, the accuracy of sleep and timer system calls
is *no longer* constrained by the jiffy, but instead can be as
accurate as the hardware allows (microsecond accuracy is typical
of modern hardware). You can determine whether high-resolution
timers are supported by checking the resolution returned by a call
to clock_getres() or looking at the "resolution" entries in
/proc/timer_list.
HRTs are not supported on all hardware architectures. (Support is
provided on x86, arm, and powerpc, among others.)
 ** The Epoch and calendar time **
UNIX systems represent time in seconds since the Epoch, 1970/01/01
00:00:00 +0000 (UTC). A program can determine the calendar time
using gettimeofday(), which returns time (in seconds and microseconds)
that have elapsed since the Epoch; time() provides similar information,
but only with accuracy to the nearest second.
// Epoch in microseconds since 1970/01/01 00:00:00.
static const uint64_t EPOCH_DELTA = 0x00dcddb30f2f8000ULL;
在32位Linux系统中，time_t是一个有符号整数，可以表示的日期范围从
1901年12月13日20时45分52秒至2038年1月19日03:14:07（SUSv3未定义
time_t值为负值的含义）。因此当前许多32位UNIX系统都面临一个2038
年的理论问题，如果执行的计算涉及未来日期，那么问题会在2038年之前
就会遭遇。事实上，到了2038年可能所有UNIX系统都早已升级为64位或更
高位系统，这一问题也随之缓解。但32为嵌入式系统，由于其寿命较之台
式机硬件更长，故而仍然会受此问题困扰。此外对于依然以32位time_t格
式保存时间的历史数据和应用程序，仍然是一个问题。
 ** Sleeping, timer and timer slack **
Various system calls and functions (nanosleep, clock_nanosleep, sleep)
allow a program to sleep for a specified period of time. And various
system calls (alarm, getitimer, timerfd_create, timer_create) allow a
process to set a timer that expires at some point in the future, and
optionally at repeated intervals.
Since Linux 2.6.28, it is possible to control the "timer slack" value
for a thread. The timer slack is the length of time by which the kernel
may delay the wake-up of certain system calls that block with a timeout.
Permitting this delay allows the kernel to coalesce wake-up events, thus
possibly reducing the number of system wake-ups and saving power. For
more description of PR_SET_TIMERSLACK in prctl().
 ** Clock function on MAC OSX **
The function clock_gettime() is not available on OSX. The mach kernel
provides three clocks: SYSTEM_CLOCK returns the time since boot time,
CALENDAR_CLOCK returns the UTC time since 1970/01/01, REALTIME_CLOCK is
deprecated and is the same as SYSTEM_CLOCK in its current implementation.
The documentation for clock_get_time() on OSX says the clocks are
monotonically incrementing unless someone calls clock_set_time(). Calls
to clock_set_time() are discouraged as it could break the monotonic
property of the clocks, and in fact, the current implementation returns
KERN_FAILURE without doing anything.
stackoverflow.com/questions/11680461/monotonic-clock-on-osx
opensource.apple.com/source/xnu/xnu-344.2/osfmk/mach/clock_types.h
#include <mach/clock.h>
#include <mach/mach.h>
clock_serv_t cclock;
mach_timespec_t mts;
host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
clock_get_time(cclock, &mts);
mach_port_deallocate(mach_task_self(), cclock);
 ** Things need to make sure **
What time is measured by the clock?
What is the precison of the clock?
How much time does the clock function spend?
How much time does the clock wrap around?
Is the clock monotonic (affected by time adjust, NTP)?
How this clock function supported between implementations?
How is the clock affected when the system suspended?
 ** References **
UEAP 6.10; USPM 10, 23; */

static struct cctime llsystime() {
  /** gettimeofday **
  #include <sys/time.h>
  int gettimeofday(struct timeval* tv, struct timezone* tz);
  @tv is a struct timeval and gives the number of
  seconds and microseconds since the Epoch. Note that the
  returned time is affected by discontinuous jumps in the system
  time (e.g., if the system adminstrator manually changes the time).
  @tz is used to acquire current timezone and daylight information,
  but it normally be specified as NULL, and the use of the timezone
  structure is obsolete. 该参数唯一合法值是NULL，其他值将产生不确定的结果，
　某些平台可能支持这个参数但这完全依赖实现，SUS对此并没有定义
　struct timezone {　int tz_minuteswest; // minutes west of Greenwich
  int tz_dsttime; // type of DST correction　};
  @@SUSv4指定该函数已被弃用，但一些程序仍然使用这个函数，因为与time
  相比，gettimeofday提供了更高的微妙级时间精度。And POSIX.1-2008
  marks gettimefoday() as obsolete, recommending the use of
  clock_gettime() instead. */
  struct timeval tv = {0};
  if (gettimeofday(&tv, 0) != 0) {
    ccloge("gettimeofday %s", strerror(errno));
    return (struct cctime){0,0,0};
  }
  return (struct cctime){(_int)tv.tv_sec, (medit)tv.tv_usec*1000, 0};
}

#if defined(CC_OS_APPLE)
#else
static struct cctime llgettime(clockid_t id) {
  struct timespec spec = {0};
  if (clock_gettime(id, &spec) != 0) {
    ccloge("clock_gettime %d %s", id, strerror(errno));
    return (struct cctime){0,0,0};
  }
  return (struct cctime){(_int)spec.tv_sec, (medit)spec.tv_nsec, 0};
}

struct cctime ccthreadtime() {
  return llgettime(CLOCK_THREAD_CPUTIME_ID);
}

struct cctime ccprocesstime() {
  return llgettime(CLOCK_PROCESS_CPUTIME_ID);
}

static _int llgetres(clockid_t id) {
  struct timespec spec = {0};
  if (clock_getres(id, &spec) != 0) {
    ccloge("clock_getres %s", strerror(errno));
    return 0;
  }
  return ((_int)spec.tv_sec)*1000000000LL + (_int)spec.tv_nsec;
}

_int ccgetsres() {
  return llgetres(CLOCK_REALTIME);
}

_int ccgetbres() {
  clockid_t id = CLOCK_MONOTONIC;
#ifdef CC_OS_LINUX
  id = CLOCK_BOOTTIME;
#endif
  return llgetres(id);
}

_int ccgettres() {
  return llgetres(CLOCK_THREAD_CPUTIME_ID);
}

_int ccgetpres() {
  return llgetres(CLOCK_PROCESS_CPUTIME_ID);
}
#endif

struct cctime ccgettime() {
#if defined(CC_OS_APPLE)
  return llsystime();
#else
  return llgettime(CLOCK_REALTIME);
#endif
}

static void llsetyear(struct ccdate* date, _int year, _int yday) {
  byte sign = 0;
  if (year < 0) { year = -year; sign = 2; }
  if (yday < 1 || yday > 366) {
    yday = (yday < 1 ? 1 : 366);
    ccloge("tm_yday invalid %s", ccitos(yday));
  }
  date->yearlow = (umedit)(year & 0xFFFFFFFF);
  date->ydaylow = (byte)(yday & 0xFF);
  date->high = (((byte)((year >> 30) & 0xFC)) | sign | ((byte)((yday >> 8) & 0x01)));
}

static struct ccdate llgetdate(time_t secs) {
  struct tm st;
  struct ccdate date = {0};
  cczero(&st, sizeof(struct tm));
  /* struct tm* gmtime_r(const time_t* secs, struct tm* out);
  The function converts the time secs to broken-down time representation,
  expressed in Coordinated Universal Time (UTC), i.e. since Epoch. */
  if (gmtime_r(&secs, &st) != &st) { /* gmtime_r needs _POSIX_C_SOURCE >= 1 */
    ccloge("gmtime_r %s", strerror(errno));
    return date;
  }
  /* tm_year - number of years since 1900, tm_yday - from 0 to 365 */
  llsetyear(&date, st.tm_year + 1900, st.tm_yday + 1);
  date.month = (int8)(st.tm_mon + 1); /* tm_mon - from 0 to 11 */
  date.wday = (int8)st.tm_wday; /* tm_wday - from 0 to 6, 0 is Sunday */
  /* in many implementations, including glibc, a 0 in tm_mday is
  interpreted as meaning the last day of the preceding month.
  这里的意思是，在将分解结构tm转换成time_t时（例如mktime），如果将
  tm_mday设为0则表示当前天数是前一个月的最后一天 */
  date.day = (int8)st.tm_mday; /* tm_mday - from 1 to 31 */
  date.hour = (int8)st.tm_hour; /* tm_hour - from 0 to 23 */
  date.min = (int8)st.tm_min; /* tm_min - from 0 to 59 */
  date.sec = (int8)st.tm_sec; /* tm_sec - from 0 to 60, 60 can be leap second */
  /* Single UNIX Specification 的以前版本允许双闰秒，于是tm_sec值的有效范围是
  0到61。但是UTC的正式定义不允许双闰秒，所以现在tm_sec值的有效范围是0到60。*/
  if (st.tm_isdst > 0) {
    /* daylight saving time is in effect. gmtime_r将时间转换成协调统一时间的分解
    结构，不会像localtime_r那样考虑本地时区和夏令时，该值应该在此处无效 */
    cclogw("gmtime_r invalid tm_isdst");
  }
  return date;
}

_int ccgetyear(struct ccdate* date) {
  _int year = (((_int)(date->high >> 2)) << 32) | ((_int)date->yearlow);
  if (date->high & 0x02) { year = -year; }
  return year;
}

_int ccgetyday(struct ccdate* date) {
  return (((_int)(date->high & 0x01)) << 8) | ((_int)date->ydaylow);
}

struct ccdate ccgetdate() {
  return ccdatefrom(ccgettime());
}

struct ccdate ccdatefromi(_int utcsecs) {
  return llgetdate(utcsecs);
}

struct ccdate ccdatefrom(struct cctime utc) {
  struct ccdate date = llgetdate(utc.sec);
  date.nsec = utc.nsec;
  return date;
}

struct ccdate ccmovetzone(struct ccdate date, int mins) {

}

struct cctime ccmonotime() {
  /** clock_gettime **
  #include <sys/time.h>
  int clock_gettime(clockid_t id, struct timespec* out);
  @CLOCK_REALTIME时clock_gettime提供了与time函数相同的功能，不
  过当系统支持高精度时间时，clock_gettime返回精度更高的实时系统时间；
  System-wide clock that measures real (i.e., wall-clock) time.
  Setting this clock requires appropriate privilege. This clock is
  affected by discontinuous jumps in the system time (e.g., if the
  system adaministrator manually changes the clock), and by the
  incremental adjustments performed by adjtime and NTP.
  @CLOCK_MONOTONIC cannot be set and represents monotonic time
  since some unspecified starting point. This clock is not
  affected by discontinuous jumps in the system time, but is
  affected by the incremental adjustments performed by adjtime
  and NTP.
  @CLOCK_BOOTTIME (since Linux 2.6.39, Linux-specific) is
  identical to CLOCK_MONOTONIC, except it also includes any time
  that the system is suspended. This allows applications to get
  a suspend-aware monotonic clock without having to deal with
  the complications of CLOCK_REALTIME, which may have discontinuities
  if the time is changed using settimeofday or similar.
  @@OSX doesn't support this function */
#ifdef CC_OS_APPLE
  return llsystime();
#else
  clockid_t id = CLOCK_MONOTONIC;
#ifdef CC_OS_LINUX
  id = CLOCK_BOOTTIME;
#endif
  return llgettime(id);
#endif
}

_int ccfilesize(struct ccfrom name) {
  /** lstat **
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  int lstat(const char* path, struct stat* st);
  It returns information about a file, in the struct stat. lstat() is
  identical to stat(), except that if path is a symbolic link, then it
  returns information about the link itself, not the file it refers to.
  struct stat {
    dev_t st_dev; // ID of device containing file
    ino_t st_ino; // inode number
    mode_t st_mode; // file type and mode
    nlink_t st_nlink; // number of hard links
    uid_t st_uid; // user ID of owner
    gid_t st_gid; // group ID of owner
    dev_t st_rdev; // device ID (if special file)
    off_t st_size; // total size, in bytes
    blksize_t st_blksize; // blocksize for filesystem I/O
    blkcnt_t st_blocks; // number of 512B blocks allocated
    // Since Linux 2.6, the kernel supports nanosecond precision for the
    // following timestamp fields.
    struct timespec st_atim; // time of last access
    struct timespec st_mtim; // time of last modification
    struct timespec st_ctim; // time of last status change
    #define st_atime st_atim.tv_sec // backward compatibility
    #define st_mtime st_mtim.tv_sec
    #define st_ctime st_ctim.tv_sec
  };
  The order of fields in the stat structure varies somewhat across
  architectures. In addition, the definition above does not show the padding
  bytes that may be present between some fields on various architectures.
  For performance and simplicity reasons, different fields in the stat
  structure may contain state infromation from different moments during the
  execution of the system call. For ecample, if st_mode or st_uid is changed
  by another process by calling chmod, or chown, stat() might return the old
  st_mode together withe the new st_uid, or the old st_uid together with the
  new st_mode.
  Not all of the Linux filesystem implement all of the time fields. And some
  filesystem types allow mounting in such a way that file and/or directory
  accesses do not cause an update of the st_atime field (see noatime, nodiratime,
  and relatime in mount(8), and related information in mount(2)). In addition,
  st_atime is not updated if a file is opened with the O_NOATIME, see open(2).
  The field st_atime is chaned by file access, for example, by execve(2), mknod(2),
  pipe(2), utime(2), and read(2) (of more than zero bytes). Other routines, like
  mmap(2), may or may not update st_atime.
  The field st_mtime is changed by file modifications, for example, by mknod(2),
  truncate(2), utime(2), and write(2) (of more than zero bytes). Moreover, st_mtime
  of a directory is changed by the creation or deletion of files in that directory.
  The st_mtime field is *not* changed for changes in owner, group, hard link count,
  or mode.
  The field st_ctime is changed by writing or by setting inode information (i.e.,
  owner, group, link count, mode, etc.). */
  struct stat st; /* lstat needs _POSIX_C_SOURCE >= 200112L */
  if (name.len == 0) return 0;
  cczero(&st, sizeof(struct stat));
  if (lstat((const char*)name.start, &st) != 0) {
    ccloge("lstat %s %s", strerror(errno), name.start);
    return 0;
  }
  return (_int)st.st_size;
}

struct ccattr ccfileattr(struct ccfrom name) {
  struct stat st;
  struct ccattr fa = {0};
  cczero(&st, sizeof(struct stat));
  if (name.len == 0) return fa;
  if (lstat((const char*)name.start, &st) != 0) {
    ccloge("lstat %s %s", strerror(errno), name.start);
    return fa;
  } /* the time stored in stat is measured in seconds since 00:00:00 UTC, January 1, 1970 */
  fa.fsize = (_int)st.st_size;
  fa.ctime = (_int)st.st_ctime;
  fa.atime = (_int)st.st_atime;
  fa.mtime = (_int)st.st_mtime;
  fa.gid = (_int)st.st_gid;
  fa.uid = (_int)st.st_uid;
  fa.mode = (_int)st.st_mode;
  fa.isfile = S_ISREG(st.st_mode);
  fa.isdir = S_ISDIR(st.st_mode);
  fa.islink = S_ISLNK(st.st_mode);
  return fa;
}

struct ccdir ccopendir(struct ccfrom name) {
  /** opendir **
  #include <sys/types.h>
  #include <dirent.h>
  DIR* opendir(const char* name);
  It opens a directory stream corresponding to the name, and returns
  a pointer to the directory stream. The stream is positioned at the
  first entry in the directory. Filename entries can be read from a
  directory stream using readdir(3). */
  struct ccdir d = {0};
  if (name.len == 0) return d;
  if ((d.stream = opendir((const char*)name.start)) == 0) {
    ccloge("opendir %s", strerror(errno));
    return d;
  }
  return d;
}

void ccclosedir(struct ccdir* d) {
  if (d->stream) {
    if (closedir((DIR*)d->stream) ! = 0) {
      ccloge("closedir %s", strerror(errno));
    }
    d->stream = 0;
  }
}

struct ccfrom ccreaddir(struct ccdir* d) {
  /** readdir **
  struct dirent* readdir(DIR* d);
  It returns a pointer to a dirent structure representing the next
  directory entry in the directory stream pointed by d. It returns
  NULL on reaching the end of the directory stream or if an error
  occurred.
  struct dirent {
    ino_t d_ino; // Inode number
    off_t d_off; // current location, should treat as an opaque value
    unsigned short d_reclen; // length of this record
    unsigned char d_type; // type of file, not supported in all filesystem
    char d_name[256]; // null-terminated filename
  };
  The only fields in the dirent structure that are mandated by POSIX.1 is
  d_name and d_ino.  The other fields are unstandardized, and no present
  on all systems. The d_name can be at most NAME_MAX characters preceding
  the terminating null byte.
  It is recommended that applications use readdir(3) instead of readdir_r().
  Furthermore, since version 2.24, glibc deprecates readdir_r(). Thre reasons
  are: On systems where NAME_MAX is undefined, calling readdir_r() may be
  unsafe because the interface doesn't allow the caller to specify the length
  of the buffer used for the returned directory entry;
  On some systems, readdir_r() cann't read directory entries with very long
  names. When the glibc implementation encounters such a name, readdir_r()
  fails with the error ENAMETOOLONG aftet the final directory entry has been
  read. On some other systems, readdir_r() may return a success status, but
  the returned d_name field may not be null terminated or may be truncated;
  In the current POSIX.1 specification (POSIX.1-2008), readdir(3) is not
  required to be thread-safe. However, in modern implementations (including
  glibc), concurrent calls to readdir(3) that specify different directory
  streams are thread-safe. Therefore, the use of readdir_R() is generally
  unnecessary in multithreaded programs. In cases where multiple threads
  must read from the same stream, using readdir(3) with external syschronization
  is still preferable to the use of readdir_r();
  It is expected a future version of POSIX.1 will make readdir_r() obsolete,
  and require that readdir(3) be thread-safe when concurrently employed on
  different directory streams. */
  struct ccfrom s = {0};
  struct dirent* entry = 0;
  errno = 0;
  if ((entry = readdir((DIR*)d->stream)) == 0) {
    if (errno != 0) { ccloge("readdir %s", strerror(errno)); }
    return s;
  }
  s.start = entry.d_name;
  s.len = strlen(entry.d_name);
  return s;
}

typedef struct {
  void* (*start)(void*);
  void* para;
  corethread_t* thread;
} threadargs_t;
#define MAX_THREADPOOL_SIZE 128
static corethread_t threadpool[MAX_THREADPOOL_SIZE];
static uint threadpoolindex = 0;
static pthread_key_t pthread_tls_key;
static bool tlskey_created = false;
static pthread_t themainthread;
static corethread_t* gettopthread() {
  corethread_t* p = 0;
  if (threadpoolindex >= MAX_THREADPOOL_SIZE) {
    core_loge("thread reach limit %s", core_utos(MAX_THREADPOOL_SIZE));
    return 0;
  }
  p = threadpool + threadpoolindex++;
  p->index = 0;
  p->func = p->para = 0;
  core_zero(&p->impl_, sizeof(corehide_t));
  return p;
}
static void freetopthread() {
  if (threadpoolindex) threadpoolindex -= 1;
}
static pthread_t* getpthreadaddr(corethread_t* self) {
  return (pthread_t*)&(self->impl_);
}
static pthread_key_t getthreadkey() {
  if (!tlskey_created) core_loge("getthreadkey unavailable");
  return pthread_tls_key;
}
static void createthreadkey() {
  int errnum = 0;
  if (tlskey_created) return;
  errnum = pthread_key_create(&pthread_tls_key, 0);
  if (errnum == 0) { tlskey_created = true; return; }
  core_loge("createthreadkey fail: %s", strerror(errnum));
  core_zero(&pthread_tls_key, sizeof(pthread_key_t));
}
static void deletethreadkey() {
  int errnum = 0;
  if (!tlskey_created) return;
  errnum = pthread_key_delete(pthread_tls_key);
  if (errnum != 0) core_loge("deletethreadkey fail: %s", strerror(errnum));
  tlskey_created = false;
}
#define CCRECORDEDSTATENUM 80 
struct ccroutine {
  lua_State* lthread;
  lua_Function lfunc;
  ccrptoto proto;
};
struct ccstate {
  _int index;
  struct ccglobal* g;
  lua_State* lstate;
  struct cclist rlist;
  pthread_t thread;
};
struct ccstatepool {
  struct ccstate s[CCRECORDEDSTATENUM];
};
struct ccglobal {
  struct ccstate* state;
  _int nstate;
  int iofd;
};
struct ccthread* ccgetmainthread(struct ccstate* s) {
  return &s->g->a[0]->thread;
}
void ccattachstate(struct ccstate* s) {
  if (s->index > 0 && s->index < CCRECORDEDSTATENUM) {
    s->g->a[s->index] = s;
  }
}
void ccdetachstate(struct ccstate* s) {
  if (s->index > 0 && s->index < CCRECORDEDSTATENUM) {
    s->g->a[s->index] = 0;
  }
}
static bool llcreatethreadkey(pthread_key_t* key) {
  /* int pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
  although the same key may be used by different threads, the values bound
  to the key are maintained on a per-thread basis and persist for the life
  of the calling thread
  an optional destructor function may be associated with each key value, at
  thread exit, if both the destructor and the value of the key are non-null,
  this destructor is called and the value of the key is set to null
  this function should called one time for one key, and it is the responsibility
  of the programmer to ensure that it is called exactly once per key before
  use of it, a typically way is to call it in the main thread */
  int n = pthread_key_create(key, 0);
  if (n != 0) { ccloge("llcreatethreadkey %s", strerror(n)); return false; }
  return true;
}
static void lldeletethreadkey(pthread_key_t key) {
  int n = pthread_key_delete(key);
  if (n != 0) { ccloge("lldeletethreadkey %s", strerror(n)); }
}
static void* llgetthreadspecific(pthread_key_t key) {
  return pthread_getspecific(key);
}
static void llsetthreadspecific(pthread_key_t key, const void* value) {
  /* different threads may bind different values to the same key, the value
  is typically a pointer to blocks of dynamically allocated memory that have
  been reserved for use by the calling thread */
  int n = pthread_setspecific(key, value);
  if (n != 0) { ccloge("llsetthreadspecific %s", strerror(n)); }
}
static void sscreatethreadkey(struct ccglobal* g) {
  if (g->keycreated) return;
  if (llgetthreadspecific(&g->key)) { g->keycreated = true; }
}
static void ssdeletethreadkey(struct ccglobal* g) {
  if (!g->keycreated) return;
  lldeletethreadkey(g->key);
  g->keycreated = false;
}
static int ssgetthreadspecific(struct ccglobal* g) {
  return (int)llgetthreadspecific(g->key);
}
static void sssetthreadspecific(struct ccglobal* g, int n) {
  llsetthreadspecific(g->key, (const void*)n); /* valid n is non-zero */
}

bool cctlskeyinit(struct cctlskey* self) {
  int n = pthread_key_create((pthread_key_t*)self, 0);
  if (n != 0) {
    ccloge("pthread_key_create %s", strerror(n));
  }
  return (n == 0);
}

void cctlskeyfree(struct cctlskey* self) {
  int n = pthread_key_delete((pthread_key_t*)self);
  if (n != 0) {
    ccloge("pthread_key_delete %s", strerror(n));
  }
}

bool cctlskeyset(struct cctlskey* self, const void* data) {
  /* different threads may bind different values to the same key, the value
  is typically a pointer to blocks of dynamically allocated memory that have
  been reserved for use by the calling thread.
  ENOMEM - insufficient memory exists to associate the value with the key.
  EINVAL - the key value is invalid. */
  int n = pthread_setspecific((pthread_key_t*)self, data);
  if (n != 0) {
    ccloge("pthread_setspecific %s", strerror(n));
  }
  return (n == 0);
}

void* cctlskeyget(struct cctlskey* self) {
  /* no errors are returned from pthread_getspecific() */
  return pthread_getspecific((pthread_key_t*)self);
}

int ccnodemain(struct ccstate* s) {
  struct ccglobal G;
  ccinitglobal(&G, "ccnode.conf");
  struct ccstate s[G.workers+1];
  int i = 0;
  ccinitstatepool(&G, s+1, G.workers);
  for (; i < G.workers + 1; ++i) {

  }
  ccinitstate(&s[0], G, pthread_self());
  G.iofd = llepollcreate();
  if (G.iofd == -1) return -1;
}
int ccstartmainthread(int (*entry)(struct ccstate* s), int argc, char** argv) {
  int n = 0;
  struct ccglobal g;
  struct ccstate s;
  cczero(&g, sizeof(struct ccglobal));
  cczero(&s, sizeof(struct ccstate));
  g.a[0] = &s;
  s.g = &g;
  s.thread.self = pthread_self();
  s.thread.func = entry;
  sscreatethreadkey(&g);
  n = entry(&s);
  ssdeletethreadkey(&g);
  return n;
}
struct ccthreadpara {
  struct ccstate* from;
  int (*func)(struct ccstate* s);
  void* para;
  pthread_t thread;
  int index;
};
static void* ccthreadfunc(void* para) {
  int n = 0;
  struct ccthreadpara* p = (struct ccthreadpara*)para;
  struct ccstate s;
  cczero(&s, sizeof(struct ccstate));
  s.g = p->from->g;
  s.index = p->index;
  s.thread.self = p->thread;
  s.thread.func = p->func;
  s.thread.para = p->para;
  ccattachstate(&s);
  n = p->func(&s);
  ccdetachstate(&s);
  return (void*)n;
}
static bool sscreatethread(struct ccstate* from, int (*func)(struct ccstate* s), void* para) {
  int n = 0, i = 0;
  struct ccthreadpara p;
  if (!pthread_equal(pthread_self(), ccgetmainthread(from)->self)) {
    ccloge("sscreatethread invalid");
    return false;
  }
  p.from = from;
  p.func = func;
  p.para = para;
  for (i = 1; i < CCRECORDEDSTATENUM; ++i) {
    if (from->g->a[i] == 0) break;
  }
  if (i == CCRECORDEDSTATENUM) { i = ++from->g->nextnum; }
  p.index = i;
  if ((n = pthread_create(&p.thread, 0, ccthreadfunc, &p)) == 0) { return true; }
  ccloge("sscreatethread %s", strerror(n));
  if (i == CCRECORDEDSTATENUM) { --from->g->nextnum; }
  return false;
}
int core_startmainthread(int (*entry)(), int argc, char** argv) {
  int errnum = 0;
  int exitcode = 0;
  createthreadkey();
  themainthread = pthread_self();
  corethread_t* thread = gettopthread();
  thread->func = entry;
  thread->para = 0;
  *getpthreadaddr(thread) = themainthread;
  errnum = pthread_setspecific(getthreadkey(), thread);
  if (errnum != 0) {
    core_loge("setspecific fail: %s", strerror(errnum));
    if (thread) freetopthread();
    return 0;
  }
  core_setcmdline(argc, argv);
  exitcode = entry();
  deletethreadkey(); /* if exit() is called or the main thread returned in the main() */
  return exitcode; /* then all threads are terminated in the process */
}
static void* corethreadfunc(void* para) {
  int errnum = 0;
  threadargs_t* args = (threadargs_t*)para;
  corethread_t* thread = args->thread;
  thread->func = args->start;
  thread->para = args->para;
  errnum = pthread_setspecific(getthreadkey(), thread);
  if (errnum != 0) {
    core_loge("setspecific fail: %s", strerror(errnum));
    return 0;
  }
  return args->start(args->para);
}
corethread_t* core_threadstart(void* (*start)(void*), void* para) {
  int errnum = 0;
  threadargs_t args;
  corethread_t* self = 0;
  if (!pthread_equal(pthread_self(), themainthread)) {
    core_loge("threadstart not in main thread");
    return 0;
  }
  args.start = start;
  args.para = para;
  args.thread = gettopthread();
  self = args.thread;
  core_zero(getpthreadaddr(self), sizeof(pthread_t));
  errnum = pthread_create(getpthreadaddr(self), 0, corethreadfunc, &args);
  if (errnum != 0) {
    core_loge("threadcreate fail: %s", strerror(errnum));
    if (self) freetopthread();
    return 0;
  }
  return self;
}
corethread_t* core_threadself() {
  corethread_t* self = (corethread_t*)pthread_getspecific(getthreadkey());
  if (!self) { core_loge("threadself fail"); return 0; }
  if (!pthread_equal(pthread_self(), *getpthreadaddr(self))) { core_loge("threadself mismatch"); return 0; }
  return self;
}
void core_threadsleep(uint us) {
  struct timespec req;
  req.tv_sec = (time_t)(us/1000000);
  req.tv_nsec = (long)(us%1000000*1000);
  if (nanosleep(&req, 0) != 0) {
    if (errno != EINTR) core_loge("threadsleep fail %s", strerror(errno));
  }
}
void core_threadexit() {
  pthread_exit((void*)1);
}
void* core_threadjoin(corethread_t* thread) {
  void* exitcode = 0;
  int errnum = pthread_join(*getpthreadaddr(thread), &exitcode);
  if (errnum != 0) {
    core_loge("threadjoin fail: %s", strerror(errnum));
    return 0; /* join a alreay joined thread will results in undefined behavior */
  } /* wait thread terminate, if already terminated then return immediately */
  return exitcode; /* the thread needs to be joinable */
}
typedef union {
  coremutex_t core;
  pthread_mutex_t lock;
} mutex_t;
coremutex_t core_mutexcreate() {
  mutex_t mutex;
  int errnum = pthread_mutex_init(&mutex.lock, 0);
  if (errnum != 0) {
    core_loge("mutexcreate fail: %s", strerror(errnum));
    core_zero(&mutex, sizeof(mutex_t));
    return mutex.core;
  }
  return mutex.core;
}
static pthread_mutex_t* getpthreadmutex(coremutex_t* self) {
  return &(((mutex_t*)self)->lock);
}
void core_mutexdestroy(coremutex_t* self) {
  int errnum = pthread_mutex_destroy(getpthreadmutex(self));
  if (errnum != 0) {
    core_loge("mutexdestroy fail: %s", strerror(errnum));
  }
}
bool core_mutexlock(coremutex_t* self) {
  int errnum = pthread_mutex_lock(getpthreadmutex(self));
  if (errnum != 0) {
    core_loge("mutexlock fail: %s", strerror(errnum));
    return false;
  }
  return true;
}
bool core_mutextrylock(coremutex_t* self) {
  int errnum = pthread_mutex_trylock(getpthreadmutex(self));
  if (errnum != 0) {
    core_loge("mutextrylock fail: %s", strerror(errnum));
    return false;
  }
  return true;
}
void core_mutexunlock(coremutex_t* self) {
  int errnum = pthread_mutex_unlock(getpthreadmutex(self));
  if (errnum != 0) {
    core_loge("mutexunlock fail: %s", strerror(errnum));
  }
}
typedef union {
  corerwlock_t core;
  pthread_rwlock_t lock;
} rwlock_t;
corerwlock_t core_rwlockcreate() {
  rwlock_t rwlock;
  int errnum = pthread_rwlock_init(&rwlock.lock, 0);
  if (errnum != 0) {
    core_loge("rwlockcreate fail: %s", strerror(errnum));
    core_zero(&rwlock, sizeof(rwlock_t));
    return rwlock.core;
  }
  return rwlock.core;
}
static pthread_rwlock_t* getpthreadrwlock(corerwlock_t* self) {
  return &(((rwlock_t*)self)->lock);
}
void core_rwlockdestroy(corerwlock_t* self) {
  int errnum = pthread_rwlock_destroy(getpthreadrwlock(self));
  if (errnum != 0) {
    core_loge("rwlockdestroy fail: %s", strerror(errnum));
  }
}
bool core_rwlockread(corerwlock_t* self) {
  int errnum = pthread_rwlock_rdlock(getpthreadrwlock(self));
  if (errnum != 0) {
    core_loge("rwlockread fail: %s", strerror(errnum));
    return false;
  }
  return true;
}
bool core_rwlocktryread(corerwlock_t* self) {
  int errnum = pthread_rwlock_tryrdlock(getpthreadrwlock(self));
  if (errnum != 0) {
    core_loge("rwlocktryread fail: %s", strerror(errnum));
    return false;
  }
  return true;
}
bool core_rwlockwrite(corerwlock_t* self) {
  int errnum = pthread_rwlock_wrlock(getpthreadrwlock(self));
  if (errnum != 0) {
    core_loge("rwlockwrite fail: %s", strerror(errnum));
    return false;
  }
  return true;
}
bool core_rwlocktrywrite(corerwlock_t* self) {
  int errnum = pthread_rwlock_trywrlock(getpthreadrwlock(self));
  if (errnum != 0) {
    core_loge("rwlocktrywrite fail: %s", strerror(errnum));
    return false;
  }
  return true;
}
void core_rwlockunlock(corerwlock_t* self) {
  int errnum = pthread_rwlock_unlock(getpthreadrwlock(self));
  if (errnum != 0) {
    core_loge("rwlockunlock fail: %s", strerror(errnum));
  }
}

struct cclinknode {
  void* prev;
  void* next;
};

struct ccthread {
  struct cclinknode node;
  struct ccglobal* g;
  lua_State* state;
  struct cclist worklist;
  pthread_t thread;
  ushort index;
  ushort weight;
  bool adjust;
  struct ccmutex mutex;
  struct cccondv condv;
};

#define CCMAXTHREADS 64
struct ccthreadpool {
  ccthread wait[CCMAXTHREADS];
  cclinknode head; /* static link and support weight add and sub */
};

struct cceventpool {
  /* map fd to work and robin work by time */
  cclinknode* slot;

};

#if defined(CC_OS_LINUX)

#include <unistd.h>
#include <sys/epoll.h>

#define CCSOCK_ACCEPT 0x01
#define CCSOCK_CONNET 0x02

struct ccepoll {
  int epfd;
  int n, maxlen;
  struct epoll_event ready[CCPOLL_WIAT_MAX_EVENTS];
};

struct ccworkitem;
struct ccworkcoll;

struct ccsockevent {
  int fd;
  umedit events;
  umedit waitop;
  umedit revents;
  struct workitem* ud;
};

struct ccworkitem {
  struct ccevent event;
  lua_State* coroutine;
  struct ccworkcoll* coll;
};

struct ccworkcoll {
  struct ccthread* thread;
};

struct ccevent {
  int fd;
  umedit waitop;
  umedit events;
  struct workitem* ud;
};

#define CCMSGTYPE_SOCKMSG 1
#define CCMSGFLAG_FREEMEM 0x0001
#define CCMSGFLAG_NEWCONN 0x0100
#define CCMSGFLAG_CONNEST 0x0200

struct ccmessage {
  umedit size;
  ushort type;
  ushort flag;
  void* data;
};

struct ccsockmsg {
  struct ccmessage head;
  int sockfd;
  umedit events;
};

struct ccsockmsg* ccnewsockmsg(int sockfd, umedit events, void* ud) {
  struct ccsockmsg* sm = (struct ccsockmsg*)ccrawalloc(sizeof(struct ccsockmsg));
  sm->head.size = sizeof(struct ccsockmsg);
  sm->head.type = CCMSGTYPE_SOCKMSG;
  sm->head.flag = CCMSGFLAG_FREEMEM;
  sm->head.data = ud;
  sm->sockfd = sockfd;
  sm->events = events;
  return sm;
}

void ccsocknewconn(struct ccepoll* self, int connfd, struct ccwork* work) {
  struct ccsockmsg* sm = ccnewsockmsg(connfd, 0, ud);
  sm->head.flag |= CCMSGFLAG_NEWCONN;
}

struct cceventnode {
  struct cclinknext node;
  _int timestamp;
  int fd;
  umedit events;
  struct ccwork* ud;
};

struct cchashslot {
  struct cclinknext node;
};

struct cceventpool {
  _int size;
  _int timestamp;
  struct cchashslot slot[1];
};

void cclinknextinit(struct cclinknext* node) {
  node->next = node;
}

void cclinknextinsert(struct cclinknext* node, struct cclinknext* newnode) {
  newnode->next = node->next;
  node->next = newnode;
}

struct cclinknext* cclinknextremove(struct cclinknext* node) {
  struct cclinknext* p = node->next;
  node->next = p->next;
  return p;
}

struct cceventpool* cceventpollinit(_int size) {
  struct cceventpool* p = 0;
  if (size <= 0) size = 1;
  p = (struct cceventpool*)ccrawalloc(sizeof(struct cceventpool) + size * sizeof(struct cchashslot));
  p->size = size;
  while (size > 0) {
    cclinknextinit(&(p->slot[--size].node));
  }
  return p;
}

struct ccepollmgr {
  struct cceventpool* pool;
};

void ccepollmgraddevent() {
}

void ccepolldispatch(struct ccepoll* self) {
  int i = 0, n = self->n;
  struct epoll_event* ready = self->ready;
  struct ccevent* e = 0;
  for (; i < n; ++i) {
    e = (struct ccevent*)ready[i].data.ptr;
    e->revents = (umedit)ready[i].events;
    if ((e->revents | CCEPOLLIN) && (e->waitop & CCSOCK_ACCEPT)) {
      ccsockaccept(self, e);
    }
  }
}

void ccepollinit(struct ccepoll* self) {
  self->epfd = -1;
}

bool ccepollcreate(struct ccepoll* self) {
  /** epoll_create **
  #include <sys/epoll.h>
  int epoll_create(int size);
  int epoll_create1(int flags);
  Since Linux 2.6.8, the size argument is ignored, but must be greater than zero.
  The kernel used this size hint information to initially allocate internal data
  structures describing events. Nowadays, this hint is no longer required (the
  kernel dynamically sizes the required data structures without needing the hing),
  The positive value of size is in order to ensure backward compatibility when
  new applications run on older kernels.
  epoll_create1() was added to the kernel in version 2.6.27 (epoll_create() is 2.6).
  If flags is 0 then it is the same as epoll_create() with obsolete size argument
  dropped. EPOLL_CLOEXEC can be included in flags to indicate closing fd when exec()
  a new program. 默认情况下，程序中打开的所有文件描述符在exec()执行新程序的过程中保持
  打开并有效。这通常很实用，因为文件描述符在新程序中自动有效，让新程序无需再去了解文件
  名或重新打开。但一些情况下在执行exec()前确保关闭某些特定的文件描述符。尤其是在特权
  进程中调用exec()来启动一个未知程序时，或启动程序并不需要这些已打开的文件描述符时，
  从安全编程的角度出发，应当在加载新程序之前关闭那些不必要的文件描述符。为此，内核为
  每个文件描述符提供了执行时关闭标志，如果设置了这一标志，如果调用exec()成功会自动关闭
  该文件描述符，如果调用exec()失败则文件描述符会继续保持打开。
  系统调用fork()允许一进程（父进程）创建一新进程（子进程），子进程几乎是父进程的翻版
  （可认为父进程一分为二了），它获得了父进程的栈、数据段、堆和执行文本段的拷贝。执行
  fork()时，子进程会获得父进程所有文件描述符的副本，这些副本的创建方式类似于dup()，
  这也意味着父子进程中对应的描述符均指向相同的打开文件句柄。文件句柄包含了当前文件
  偏移量以及文件状态标志，因此这些属性是在父子进程间共享的，例如子进程如果更新了文件
  偏移量，那么这种改变也会影响到父进程中相应的描述符。
  系统调用exec()用于执行一个新程序，它将新程序加载到当前进程中，浙江丢弃现存的程序段，
  并为新程序重新创建栈、数据段以及堆。
  EMFILE - The per-user limit on the number of epoll instances imposed by
  /proc/sys/fs/epoll/max_user_instances was encountered.
  ENFILE - The system-wide limit on the total number of open files has been reached.
  ENOMEM - There was insufficient memory to create the kernel object. */
  ccepollclose(self);
  if ((self->epfd = epoll_create1(0)) == -1) {
    ccloge("epoll_create %s", strerror(errno));
    return false;
  }
  return true;
}

void ccepollclose(struct ccepoll* self) {
  if (self->epfd == -1) return;
  if (close(self->epfd) != 0) {
    ccloge("close epoll %s", strerror(errno));
  }
  self->epfd = -1;
}

/** epoll - I/O event notification facility **
The epoll API performs a similar task to poll(2): monitoring multiple file
description to see if I/O is possible on any of them. The epoll API can be
used either as an edge-triggered or a level-triggered and scales well to
large numbers of watched file descriptions.
The difference between the ET and LT can be described as follows. Suppose
that a fd received 2KB data, a call to epoll_wait is done that will return
fd as a ready file descriptor. And then 1KB data is read from fd, than a call
to epoll_wait again. If the fd configured using ET then the call to epoll_wait
will be blocked despite the available data still present in the buffer; meanwhile
the remote peer might be expecting a response based on the data it alrady sent.
If it is, the call to epoll_wait will blcok indefinitely due to ET mode only
triggered when changes occur on the monitored file descritor.
An application that employs the EF should use nonblocking fd to avoid having a
blocking read or write starve a task that is handling multiple fds. The suggested
way to use ET epoll with nonblocking fds and by waiting for an event only after
read or write return EAGAIN.
By contracst, when used as LT, epoll is simply a faster poll. Since even with
ET epoll, multiple events can be generated upon receipt of multiple chunks of data,
the caller has the option to specify EPOLLONESHOT flag, to tell epoll to disable
the fd after the receipt of an event. It is the caller's responsibility to rearm
the fd using epoll_ctl with EPOLL_CTL_MOD.
If the system is in autosleep mode via /sys/power/autosleep and an event happens
which wakes the device from sleep, the device driver will keep the device awake
only until that event is queued. It is necessary to use the epoll_ctl EPOLLWAKEUP
flag to keep the device awake until the event has been processed. In specifically,
the system will be kept awake from the moment the event is queued, through the
epoll_wait is returned and until the subsequence epoll_wait call. If the event
should keep the system awake beyond that time, then a separate wake_lock should
be taken before the second epoll_wait call.
/proc/sys/fs/epoll/max_user_watchers (since Linux 2.6.28) specifies a limit on the
total number of fd that a user can register across all epoll instances on the
system. The limit is per real user ID. Each registered fd costs roughly 90 bytes
on a 32-bit kernel, and roughtly 160 bytes on a 64-bit kernel. Currently, the
default value for max_user_watchers is 1/25 (4%) of the available low memory,
divided by the registration cost in bytes.
The edge-triggered usage requires more clarification to avoid stalls in the
application event loop.
What happens if you register the same fd on an epoll instance twice? You will
probably get EEXIST. However, it is possible to add a duplicate fd (dup(2),
dup2(2), fcntl(2) F_DUPFD). This can be a useful technique for filtering events,
if the duplicate fd are registered with different events masks.
Can two epoll instances wait for the same fd? If so, are events reported to both
epoll fd? Yes, and events would be reported to both. However, careful programming
may be needed to do this correctly.
Is the epoll file descriptor itself poll/epoll/selectable? Yes, If an epoll fd
has events waiting, then it will indicate as being readable. What happens if one
attempts to put an epoll fd into its own fd set? The epoll_ctl call will return
EINVAL. However, you can add an epoll fd inside another epoll fd set.
Will closing a fd cause it to be removed from all epoll sets automatically? Yes,
but be aware of the following point. A fd is a reference to an open fd. Whether
a fd is duplcated via dup(2), dup2(2), fcntl(2) F_DUPFD, or fork(2), a new fd
refering to the same open fd is created. An open fd continues to exist until all
fd refering to it have been closed. A fd is removed from an epoll set only after
all the fds referring to the underlying open fd have been closed. This means that
even after a fd that is part of an epoll set has been closed, events may be reported
for that fd if other fds referring to the same underlying fd remain open.
If more the one event occurs between epoll_wait calls, are they combined? They will
be combined.
Receiving an event from epoll_wait should suggest to you that such fd is ready for
the requested I/O operation. You must consider it ready until the next (nonblocking)
read/write yields EAGAIN. When and how you will use the fd is entirely up to you.
For packet/token-orientied files (e.g., datagram socket, terminal in canonical mode),
the only way to detect the end of the read/write I/O space is to continue to read/write
until EAGAIN. For stream-oriented files (e.g., pipe, FIFO, stream socket), the
condition that the read/write I/O sapce is exhausted can also be detected by checking
the amount of data from/written to the target fd. For example, if you call read by asking
to read a certain amount of data and read returns a lower number of bytes, you can be
sure of having exhausted the read I/O space for the fd. The same is true when using write.
If there is a large amount of I/O space, it is possible that by trying to drain it the
other files will not get processed causing starvation. The solution is to maintain a
ready list and mark the fd as ready in its associated data structure, thereby allowing
the application to remember which files need to be processed but still round robin
amongst all the ready files. This also supports ignoring subsequent events you receive
for fds that are already ready.
If you use an event cache or store all the fds returned from epoll_wait, then make sure
to provide a way to mark its closure dynamically. Suppose you receive 100 events from
epoll_wait, and in event #47 a condition causes event #13 to be closed. If you remove
the structure and close the fd for event #13, then your event cached might still say
there are events waiting for that fd causing confusion.
On solution for this is to call, during the processing of event 47, epoll_ctl to delete
fd 13 and close, then mark its associated data structure as removed and link it to a
cleanup list. If you find another event for fd 13 in your batch processing, you will
discover the fd had been previously removed and there will be no confustion.
The epoll api is Linux-specific. Some other systems provided similar mechanisms, for
example, FreeBSD has kqueue, and Solaris has /dev/poll.
The set of fds that is being monitored via an epoll fd can be viewed the entry for the
epoll fd in the process's /proc/[pid]/fdinfo directory.
*/

bool ccepolladd(struct ccepoll* self, struct ccevent* e) {
  /** event masks **
  The bit masks can be composed using the following event types:
  EPOLLIN - The associated file is available for read operations.
  EPOLLOUT - The associated file is available for write operations.
  EPOLLRDHUP (since Linux 2.6.17) - Stream socket peer closed
  connection, or shut down writing half of connection. (This flag
  is especially useful for wirting simple code to detect peer
  shutdown when using Edge Triggered monitoring)
  EPOLLPRI - There is urgent data available for read operations.
  EPOLLERR - Error condition happened on the associated fd.
  epoll_wait will always wait for this event; it is not necessary
  to set it in events.
  EPOLLHUP - Hang up happened on the associated fd. epoll_wait will
  always wait for this event; it is not necessary to set it in
  events. Not that when reading from a channel such as a pipe or a
  stream socket, this event merely indicates that the peer closed
  its end of the channel. Subsequent reads from the channel will
  return 0 (end of file) only after all outstanding data in the
  channel has been consumed.
  EPOLLET - Sets the Edge Triggered behavior for the associated fd.
  The default behavior for epoll is Level Triggered.
  EPOLLONESHOT (since Linux 2.6.2) - Sets the one-shot behavior
  for the associated fd. This means that after an event is pulled
  out with epoll_wait the associated fd is internally disabled and
  no other events will be reported by the epoll interface. The
  user must call epoll_ctrl with EPOLL_CTL_MOD to rearm the fd
  with a new event mask.
  EPOLLWAKEUP (since Linux 3.5) - If EPOLLONESHOT and EPOLLET are
  clear and the process has the CAP_BLOCK_SUSPEND capability, ensure
  that the system does not enter "suspend" or "hibernate" while
  this event is pending or being processed. The event is considered
  as being "processed" from the time when it is returned by a call
  to epoll_wait until the next call to epoll_wait on the same
  epoll fd, the closure of that fd, the removal of the event fd
  with EPOLL_CTL_DEL, or the clearing of EPOLLWAKEUP for the event
  fd with EPOLL_CTL_MOD. If EPOLLWAKEUP is specified, but the caller
  does not have the CAP_BLOCK_SUSPEND capability, then this flag is
  silently ignored. A robust application should double check it has
  the CAP_BLOCK_SUSPEND capability if attempting to use this flag.
  EPOLLEXCLUSIVE (since Linux 4.5) - Sets an exclusive wakeup mode
  for the epoll fd that is being attached to the target fd. When a
  wakeup event occurs and multiple epoll fd are attached to the same
  fd using EPOLLEXCLUSIVE, one or more of the epoll fds will receive
  an event with epoll_wait. The default in this scenario (when
  EPOLLEXCLUSIVE is not set) is for all epoll fds to receive an
  event. EPOLLEXCLUSIVE is thus usefull for avoiding thundering
  herd problems in certain scenarios.
  If the same fd is in multiple epoll instances, some with the
  EPOLLEXCLUSIVE, and others without, then events will be provided
  to all epoll instances that did not specify EPOLLEXCLUSIVE, and
  at least one of the the epoll instances that did specify
  EPOLLEXCLUSIVE.
  The following values may be specified in conjunction with
  EPOLLEXCLUSIVE: EPOLLIN, EPOLLOUT, EPOLLWAKEUP, and EPOLLET.
  EPOLLHUP and EPOLLERR can also be specified, but this is not
  required: as usual, these events are always reported if they
  cocur. Attempts to specify other values yield an error.
  EPOLLEXCLUSIVE may be used only in an EPOLL_CTL_ADD operation;
  attempts to employ it with EPOLL_CTL_MOD yield an error. A call
  to epoll_ctl that specifies EPOLLEXCLUSIVE and specifies the target
  fd as an epoll instance will likewise fail. The error in all of
  these cases is EINVAL. */
  if (!llepollctl(self->epfd, EPOLL_CTL_ADD, e)) {
    if (errno == EEXIST) {
      return llepollctl(self->epfd, EPOLL_CTL_MOD, e);
    }
    return false;
  }
  return true;
}

#define CCEPOLLIN EPOLLIN
#define CCEPOLLPRI EPOLLPRI
#define CCEPOLLOUT EPOLLOUT
#define CCEPOLLERR EPOLLERR
#define CCEPOLLHUP (EPOLLHUP | EPOLLRDHUP)

bool llepollctl(int epfd, int op, struct ccevent* e) {
  struct epoll_event event;
  if (epfd == -1 || e->fd == -1 || epfd == e->fd) {
    ccloge("llepollctl invalid argument");
    return false;
  }
  /** epoll_event **
  struct epoll_event {
    uint32_t events; // epoll events
    epoll_data_t data; // user data variable
  };
  typedef union epoll_data {
    void* ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
  } epoll_data_t; */
  event.events = EPOLLET | (uint32_t)e->events;
  event.data.ptr = e;
  /** epoll_ctl **
  #include <sys/epoll.h>
  int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
  The epoll interface supports all fds that support poll(2).
  On success, it returns zero. On error, it returns -1 and errno is set.
  EBADF - epfd or fd is not a valid fd.
  EEXIST - op was EPOLL_CTL_ADD, and the supplied fd is already registered.
  EINVAL - epfd is not an epoll fd, or fd is the same as epfd, or the
  requested operation op is not supported by this interface, or an invalid
  event type was specified along with EPOLLEXCLUSIVE, or op was EPOLL_CTL_MOD
  and events include EPOLLEXCLUSIVE, or op was EPOLL_CTL_MOD and the
  EPOLLEXCLUSIVE has previously been applied to this epfd, fd pair; or
  EPOLLEXCLUSIVE was specified in event and fd refers to an epoll instance.
  ELOOP - fd refers to an epoll instance and this EPOLL_CTL_ADD would result
  in a circular loop of epoll instances monitor one another.
  ENOENT - op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not registered
  registered with this epoll instance.
  ENOMEM - there was insufficient memory to handle the requested op.
  ENOSPC - the limit imposed by /proc/sys/fs/epoll/max_user_watches was
  encountered while trying to register a new fd on an epoll instance.
  EPERM - the target file fd does not support epoll. this can occur if fd
  refers to, for example, a regular file or a directory. */
  if (epoll_ctl(epfd, op, e->fd, &event) != 0) {
    ccloge("epoll_ctl %s", strerror(errno));
    return false;
  }
  return true;
}

bool ccepollmod(struct ccepoll* self, struct ccevent* e) {
  return llepollctl(self->epfd, EPOLL_CTL_MOD, e);
}

bool ccepolldel(struct ccepoll* self, int fd) {
  /* In kernel versions before 2.6.9, the EPOLL_CTL_DEL
  operation required a non-null pointer in event, even
  though this argument is ignored. Since Linux 2.6.9, event
  can be specified as NULL when using EPOLL_CTL_DEL.
  Applications that need to be portable to kernels before
  2.6.9 should specify a non-null pointer in event. */
  struct ccevent e;
  cczero(&e, sizeof(struct ccevent));
  e.fd = fd;
  return llepollctl(self->epfd, EPOLL_CTL_DEL, &e);
}

bool llepollwait(struct ccepoll* self, int ms) {
  /** epoll_wait **
  #include <sys/epoll.h>
  int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
  The timeout argument specifies the number of milliseconds that epoll_wait will block.
  Time is measured against the CLOCK_MONOTONIC clock. The call will block until either:
  * a fd delivers an event;
  * the call is interrupted by a signal handler; or
  * the timeout expires.
  Note that the timeout interval will be rounded up to the system clock granularity,
  and kernel scheduling delays mean that the blocking interval may overrun by a small
  amount. Specifying a timeout of -1 causes epoll_wait() to block indefinitely, while
  specifying a timeout of 0 causes epoll_wait() to return immediately, even if no events
  are available.
  In kernel before 2.6.37, a timeout larger than approximately LONG_MAX/HZ ms is treated
  as -1 (i.e., infinity). Thus, for example, on a system where the sizeof(long) is 4 and
  the kernel HZ value is 1000, this means that timeouts greater than 35.79 minutes are
  treated as infinity.
  ---
  On success, it returns the number of fds ready for the requested I/O, or 0 if no fd became
  ready during the requested timeout milliseconds. When an error occurs, epoll_wait() returns
  -1 and errno is set appropriately.
  EBADF - epfd is not a valid fd
  EFAULT - the memory data pointed by events is not accessible with write permission.
  EINTR - the call was interrupted by a signal handler before either any of the requested
  events occured or the timeout expired.
  EINVAL - epfd is not an epoll fd, or maxevents is less than or equal to 0.
  ---
  While one thread is blocked in a call to epoll_wait(), it is possible for another thread
  to add a fd to the waited-upon epoll instance. If the new fd becomes ready, it will cause
  the epoll_wait() call to unblock.
  For a discussion of what may happen if a fd in an epoll instance being monitored by epoll_wait()
  is closed in another thread, see select(2).
  If a fd being monitered by select() is closed in another thread, the result is unspecified.
  On some UNIX systems, select() unblocks and returns, with an indication that the fd is ready
  (a subsequent I/O operation will likely fail with an error, unless another the fd reopened
  between the time select() returned and the I/O operations was performed). On Linux (and some
  other systems), closing the fd in another thread has no effect on select(). In summary, any
  application that relies on a particular behavior in this scenario must be considered buggy.
  Under Linux, select() may report a socket fd as "ready for reading", while nevertheless a
  subsequent read blocks. This could for example happen when data has arrived but upon
  examination has wrong checksum and is discarded. There may be other circumstances in which
  a fd is spuriously reported as ready. Thus it may be safer to use O_NONBLOCK on sockets that
  should not block. */
  for (; ;) {
    if ((self->n = epoll_wait(self->epfd, self->receive, self->maxlen, ms)) >= 0) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    ccloge("epoll_wait %s", strerror(errno));
    return false;
  }
  return true;
}

bool ccepollwait(struct ccepoll* self) {
  return llepollwait(self, -1);
}

bool ccepolltrywait(struct ccepoll* self) {
  return llepollwait(self, 0);
}

bool ccepollwaitms(struct ccepoll* self, int ms) {
  if (ms > 30 * 60 * 1000) ms = 30 * 60 * 1000;
  return llepollwait(self, ms);
}

#else

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
#include <sys/types.h>
#include <sys/event.h>
#else
#include <poll.h>

struct ccepoll {
  struct ccarray fdset;
  struct cchashtbl fdud;
};

#define CCEPOLLINITFDBITS 6  /* 2 ^ 6 = 64 */
#define CCEPOLLINITFDSIZE (1 << CCEPOLLINITFDBITS)

void ccepollinit(struct ccepoll* self) {
  cczero(self, sizeof(struct ccepoll));
}

bool ccepollcreate(struct ccepoll* self) {
  ccarrayinit(&self->fdset, sizeof(struct pollfd), CCEPOLLINITFDSIZE);
  cchashtblinit(&self->fdud, CCEPOLLINITFDBITS);
  return true;
}

void ccepollclose(struct ccepoll* self) {
  ccarrayfree(&self->fdset);
  cchashtblfree(&self->fdud);
}

#define CCEPOLLIN POLLIN
#define CCEPOLLPRI POLLPRI
#define CCEPOLLOUT POLLOUT
#define CCEPOLLERR (POLLERR | POLLNVAL)
#define CCEPOLLHUP POLLHUP

bool ccepolladd(struct ccepoll* self, int fd, char flag) {
  struct pollfd event;
  event.fd = fd;
  event.revents = 0;
  event.events = POLLIN | POLLPRI;
  if (flag == 'a') {
    event.events |= POLLOUT;
  } else if (flag == 'w') {
    event.events = POLLOUT;
  } else if (flag != 'r') {
    event.events = 0;
    ccloge("epolladd invalid falg");
    return false;
  }
  return ccarrayadd(&self->fdset, &event);
}

bool ccepollmod(struct ccepoll* self, int fd, char flag) {
  struct pollfd event = {0};
  event.fd = fd;
  struct pollfd* p = ccarrayfind(&self->fdset, fd, ccepollequalfunc);
  if (p == 0) return;
  p->revents = 0;
  p->events = POLLIN | POLLPRI;
  if (flag == 'a') {
    event.events |= POLLOUT;
  } else if (flag == 'w') {
    event.events = POLLOUT;
  } else if (flag != 'r') {
    event.events = 0;
    ccloge("epollmod invalid flag");
  }
}

void ccepolldel(struct ccepoll* self, int fd) {
  struct pollfd event = {0};
  event.fd = fd;
  ccarraydel(&self->fdset, &event, ccepollequalfunc);
}

void ccepollwait(struct ccepoll* self) {
  ccepollwaitms(self, INFTIM);
}

void ccepolltrywait(struct ccepoll* self) {
  ccepollwaitms(self, 0);
}

void ccepollwaitms(struct ccepoll* self, int ms) {
  poll(self->fdset.buffer, self->fdset.nelem, ms);
}

#endif

#endif

struct cctcpserver {
  int listenfd;
};

union llsockaddr {
  struct sockaddr_storage storage;
  struct sockaddr sa;
  struct sockaddr_in in;
  struct sockaddr_in6 in6;
};

int ccsockpton(struct ccfrom ip, ushort port, union llsockaddr* dest) {
  /** inet_pton - convert ipv4 and ipv6 addresses from text to binary form **
  #include <arpa/inet.h>
  int inet_pton(int family, const char* str, void* dest);
  AF_INET - src contains an ipv4 network address in dotted-decimal format,
  "ddd.ddd.ddd.ddd", where ddd is a decimal number of up to three digits in
  the range 0 to 255. The address is convered to a struct in_addr and copied
  to dest, which must be sizeof(struct in_addr) long.
  AF_INET6 - src contains an ipv6 network address. The address is cnverted to
  and struct in6_addr and copied to dest, which must be sizeof(struct in6_addr)
  long. The allowed formats for ipv6 addresses are: x:x:x:x:x:x:x:x, each x can
  be up to 4 hex digits; A series of contiguous zero values in the preferred
  format can be abbreviated to ::, only one instance of :: can occur in an
  address. For example, the loopback address 0:0:0:0:0:0:0:1 can be abbreviated
  as ::1, the wildcard address, consisting of all zeros, can be written as ::;
  An alternate format is useful for expressing ipv4-mapped ipv6 address. This
  form is written as x:x:x:x:x:x:d.d.d.d, an example of such an address is
  ::FFFF:204.152.189.116.
  inet_pton() returns 1 on success, 0 is returned if src does not contain a
  character string representing a valid network in the specified address family.
  If family does not contain a valid address family, -1 is returned and errno is
  set to EAFNOSUPPORT. */
  if (ccstrcontain(ip, ':')) {
    struct in6_addr addr;
    if (inet_pton(AF_INET6, (const char*)ip.start, &addr) != 1) {
      return 0;
    }
    cczero(dest, sizeof(union llsockaddr));
    dest->in6.sin6_family = AF_INET6;
    dest->in6.sin6_port = htons(port);
    dest->in6.sin6_addr = addr;
    return sizeof(struct sockaddr_in6);
  } else {
    struct in_addr addr;
    if (inet_pton(AF_INET, (const char*)ip.start, &addr) != 1) {
      return 0;
    }
    cczero(dest, sizeof(union llsockaddr));
    dest->in.sin_family = AF_INET;
    dest->in.sin_port = htons(port);
    dest->in.sin_addr = addr;
    return sizeof(struct sockaddr_in);
  }
}

_int ccsockntop(const union llsockaddr* sa, socklen_t len, struct ccstring* ip, ushort* port) {
  /** inet_ntop - convert ipv4 and ipv6 addresses from binary to text form **
  #include <arpa/inet.h>
  const char* inet_ntop(int family, const void* src, char* dest, socklen_t size);
  This function converts the network address src in the family into a character
  string. The resulting is copied to the buffer pointed to by dest, which must
  be a non-null pointer. The caller specifies the number of bytes available in
  this buffer in size.
  AF_INET - src points to a struct in_addr (in network byte order) which is
  converted to an ipv4 network address in the dotted-decimal format, "ddd.ddd.ddd.ddd".
  The buffer dest must be at least INET_ADDRSTRLEN bytes long.
  AF_INET6 - src points to a struct in6_addr (in network byte order) which is
  converted to a representation of this address in the most appropriate ipv6
  network address format for this address. The buffer dst must be at least
  INET6_ADDRSTRLEN bytes long.
  On success, inet_ntop() returns a non-null pointer to dst. NULL is returned if
  there was an error, with errno set to indicate the error. */
  #define LLIPSTRMAXLEN 128
  char dest[LLIPSTRMAXLEN+1] = {0};
  ccstrsetempty(ip);
  *port = 0;
  if (sa == 0 || len <= 0) return EINVAL;
  if (sa->sa.sa_family == AF_INET) {
    if (inet_ntop(AF_INET, &(sa->in.sin_addr), dest, LLIPSTRMAXLEN) == 0) {
      return errno;
    }
    *port = ntohs(sa->in.sin_port);
    ccstrsetfromc(ip, dest);
    return 0;
  }
  if (sa->sa.sa_family == AF_INET6) {
    if (inet_ntop(AF_INET6, &(sa->in6.sin6_addr), dest, LLIPSTRMAXLEN) == 0) {
      return errno;
    }
    *port = ntohs(sa->in6.sin6_port);
    ccstrsetfromc(ip, dest);
    return 0;
  }
  #undef LLIPSTRMAXLEN
  ccloge("sockntop invalid family");
  return EINVAL;
}

bool ccsockbind(int sockfd, ccfrom ip, ushort port) {
  /** bind - bind a address to a socket **
  #include <sys/types.h>
  #include <sys/socket.h>
  int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
  It is normally necessary to assign a local address using bind() before
  SOCK_STREAM socket may receive connections. The actual structure passed
  to the addr argument will depend on the address family. The only purpose
  of struct sockaddr is to do the cast.
  On success, zero is returned. On error, -1 is returned and errno is set.
  EACCES - the address is portected, and the user is not the superuser.
  EADDRINUSE - the given address is already in use.
  EBADF - sockfd is not a valid fd.
  EINVAL - the socket is already bound to an address, or addrlen is wrong
  addr is not a valid address for this socket's domain.
  ENOTSOCK - the sockfd is not refer to a socket.
  And there are other errors specific to UNIX domain (AF_UNIX) sockets.
  如果一个TCP客户或服务器未曾调用bind绑定一个端口，当使用connect或listen
  时，内核会为相应的套接字选择一个临时端口。当然这对于TCP服务器是罕见的，
  服务器通常会绑定一个众所周知的端口。一个例外是RPC服务器，它们通常由内核
  为它们的监听套接字选择一个临时端口，然后该端口会通过RPC端口映射器进行注
  册。客户在connect这些服务器之前，必须与端口映射器联系以获取它们的临时
  端口。如果由内核选择临时端口，需要调用函数getsockname()来获取。
  进程可以把一个特定的IP地址绑定到它的套接字上，这个IP地址必须属于其所在
  主机的网络接口之一。对于TCP客户，这相当于给在该套接字上发送的IP数据报
  指派了源IP地址；但客户端通常不绑定IP地址，当连接套接字时，内核根据所用
  外出网络接口来选择源IP地址，而所用外出接口则取决于到达服务器所需路径（见
  TCPv2第737页）。对于TCP服务器，这就限定该套接字只接收那些目的地址为这个
  IP地址的客户连接；如果服务器不绑定IP地址，内核就把客户发送的SYN的目的IP
  地址作为服务器的源IP地址（TCPv2第943页）。
  我们必须区分分组的的到达地址和该分组的目的IP地址，一般大多数实现都使用弱
  端系统模型，这意味着一个分组只要其目的IP地址能够匹配目的主机的某个网络接
  口就行，这个目的地址不一定是它的到达地址。绑定非通配IP地址可以根据数据包
  使用的目的IP地址来确定接下来发送数据包的源IP地址，而对到达接口不做限制，
  除非主机采用强端系统模型。
  如果绑定的IP地址是通配地址，就是告诉系统要是系统是多缩主机（有多个网络接口），
  我们将接受目的地址为任何本地接口的连接。如果绑定的端口为0则系统会自动分配
  一个临时端口使用。如果自行设置一个临时端口使用，它应该比1023大（不应该是保留
  端口），比5000大（以免与许多源自Berkeley的实现分配临时端口的范围冲突），比
  49152小（以免与临时端口号的“正确”范围冲突）。
  从bind返回的一个常见错误是EADDRINUSE，到7.5节讨论SO_REUSEADDR和SO_REUSEPORT
  这两个套接字选项时在详细讨论。*/
  union llsockaddr sa;
  socklen_t len = (socklen_t)ccsockpton(ip, port, &sa);
  if (len == 0) {
    ccloge("bind invalid ip");
    return false;
  }
  if (bind(sockfd, &sa.sa, len) != 0) {
    ccloge("bind %s", strerror(errorno));
    return false;
  }
  return true;
}

bool ccsocklisten(int sockfd, int backlog) {
  /** listen - listen for connections on a socket **
  #include <sys/types.h>
  #include <sys/socket.h>
  int listen(int sockfd, int backlog);
  It marks the socket referred to by sockfd as a passive socket,
  that is, as a socket that will be used to accept incoming
  connection requests using accept. The sockfd is a fd that refers
  to a socket of type SOCK_STREAM or SOCK_SEQPACKET.
  The backlog defines the maximum length to which the queue of
  pending connections for sockfd may grow. If a connection request
  arrives when the queue is full, the client may receive an error
  with an indication of ECONNREFUSED or, if the underlying protocol
  supports retransmission, the request may be ignored so that a
  later reattempt at connection succeeds.
  The behavior of the backlog on TCP sockets changed with Linux 2.2.
  Now it specifies the queue length for completely established sockets
  waiting to be accepted, instead of the number of incomplete connection
  requests. The maximum length of the queue for incomplete sockets can
  be set using /proc/sys/net/ipv4/tcp_max_syn_backlog. When syncookies
  are enabled there is no logical maximum length and this setting is
  ignored. See tcp(7) for more information.
  If the backlog is greater than the value in /proc/sys/net/core/somaxconn,
  then it is silently truncated to that value; the default value in this
  file is 128. In kernels before 2.4.25, this limit was a hard coded value,
  SOMAXCONN, with the value 128.
  EADDRINUSE - Another socket is already listening on the same port.
  EBADF - the sockfd is not a valid fd.
  ENOTSOCK - the sockfd is not refer to a socket.
  EOPNOTSUPP - the socket is not of a type that supports the listen() op.
  当socket函数创建一个套接字时，它被假设为一个主动套接字，即调用connect发起
  连接的客户套接字。listen函数把一个为连接的套接字转换成一个被动套接字，指示
  内核应该接受指向该套接字的连接请求。根据TCP状态图，调用listen后导致套接字
  从CLOSED状态转换成LISTEN状态。
  内核为任何给定的监听套接字维护了两个队列。一个是未完成连接队列(incomplete
  connection queue），该队列里的套接字处于SYN_RCVD状态，表示已经收到了客户发
  的SYN但是TCP三次握手还没有完成；一个是已完成队列（completed connection
  queue），其中的套接字都处于ESTABLISHED状态，TCP三次握手的连接过程已成功完成。
  当来自客户SYN到达时，TCP在未完成连接队列中创建一个新项，然后发送SYN和ACK到
  客户端。该新建项会保持在队列中直到客户端ACK到来或等待超时（源自Berkeley的
  实现这个超时值为75s）。如果成功收到客户的ACK，该项会从未完成队列移到已完成
  队列的队尾。当进程调用accept时，已完成队列中的对头项将返回给进程，或者如果
  该队列为空，那么进程将被投入睡眠直到TCP在该队列放入一项才唤醒。
  backlog的含义从未有过正式的定义，4.2BSD宣称它的定义是由未处理连接构成的队列
  可能增长到的最大长度。该定义并未解释未处理连接是处于SYNC_RCVD状态的连接，还
  是尚未由进程接受的处于ESTABLISHED状态的连接，亦或两者皆可。源自Berkeley的实现
  给backlog增设了一个模糊因子，它乘以1.5得到未处理队列最大长度。不要把backlog
  设为0，因为不同的实现对此有不同的解释，如果你不想让任何客户连接到你的监听套
  接字上，那就关闭该监听套接字。指定一个比内核能够支持的值大的backlog是可以接受
  的，内核会把所指定的偏大值截断成自己支持的最大值而不返回错误。
  当一个客户SYN到达时，或这些队列是满的，TCP就忽略该SYN，不应该发送RST。因为
  这种情况是暂时的，客户TCP将重发SYN。要是服务器TCP立即响应一个RST，客户的
  connect就会立即返回一个错误，强制客户应用程序处理这种状况而不是让TCP的重传机制
  来处理。另外，客户无法区分响应SYN的RST究竟意味着“该端口没有服务器在监听”，还是
  “该端口有服务器在监听，不过它的队列已经满了”。
  在三次握手之后，但在服务器accept之前到达的数据应该有服务器TCP排队，最大数据量
  为相应已连接套接字的接收缓冲区大小。*/
  if (listen(sockfd, backlog) != 0) {
    ccloge("listen %s", strerror(errno));
    return false;
  }
  return true;
}

struct ccsockconn {
  int connfd;
  struct ccstring localip;
  struct ccstring remoteip;
  ushort localport;
  ushort remoteport;
};

void ccsockconninit(struct ccsockconn* conn) {
  cczero(conn, sizeof(struct ccsockconn));
  conn->connfd = -1;
}

_int cclocaladdr(int connfd, struct ccstring* ip, ushort* port) {
  union llsockaddr sa;
  socklen_t len = sizeof(sa);
  cczero(&sa, sizeof(sa));
  if (getsockname(connfd, (struct sockaddr*)&sa, &len) != 0) {
    ccloge("getsockname %s", strerror(errno));
    ccstrsetempty(ip);
    *port = 0;
    return errno;
  }
  return ccsockntop(&sa, len, ip, port);
}

/** POSIX signal interrupt process's execution **
信号（signal）是告知某个进程发生了某个事件的通知，也称谓软中断
（software interrupt）。信号通常是异步发生的，也就是说进程预先
不知道信号的准确发生时机。信号可以有一个进程发送给另一个进程或
进程自身，或者由内核发给某个进程。例如SIGCHLD信号是由内核在任
何子进程终止时发给它的父进程的一个信号。
每个信号都有一个与之关联的处置（disposition），也称为行为
（action），我们通过调用sigaction函数来设定一个信号的处置，并有
三种选择。其一，可以提供一个函数，只要有特定信号发生时它就会调用。
这样的函数称为信号处理函数（signal handler），这种行为称为捕获
（catching）信号。有两个信号不能被捕获，它们是SIGKILL和SIGSTOP。
信号处理函数的原型为 void (*handle)(int signo) 。对于大多数信号
来说，调用sigaction函数并指定信号发生时所调用的函数就是捕获信号
所需要的全部工作。不过稍后可以看到，SIGIO、SIGPOLL和SIGURG这些
信号还要求捕获它们的进程做些额外工作。
其二，可以把某个信号的处置设定为SIG_IGN来忽略它，SIGKILL和SIGSTOP
这两个信号不能被忽略。其三，可以把某个信号的处置设定为SIG_DFL启用
默认处置。默认处置通常是在收到信号后终止进程，某些信号还在当前工作
目录产生一个core dump。另有个别信号的默认处置是忽略，SIGCHLD和
SIGURG（带外数据到达时发送）是默认处置为忽略的两个信号。
POSIX允许我们指定一组信号，它们在信号处理函数被调用时阻塞，任何阻塞
的信号都不能递交给进程。如果将sa_mask成员设为空集，意味着在该信号处
理期间，不阻塞额外的信号。POSIX会保证被捕获的信号在其信号处理期间总
是阻塞的。因此在信号处理函数执行期间，被捕获的信号和sa_mask中的信号
是被阻塞的。如果一个信号在被阻塞期间产生了一次或多次，那么该信号被
解阻塞之后通常只递交一次，即Unix信号默认是不排队的。利用sigprocmask
函数选择性的阻塞或解阻塞一组信号是可能的。这使得我们可以做到在一段
临界区代码执行期间，防止捕获某些信号，以保护这段代码。
警告：在信号处理函数中调用诸如printf这样的标准I/O函数是不合适的，其原
因将在11.18中讨论。在System V和Unix 98标准下，如果一个进程把SIGCHLD
设为SIG_IGN，它的子进程就不会变成僵死进程。不幸的是，这种做法仅仅适用
于System V和Unix 98，而POSIX明确表示没有这样的规定。处理僵死进程的可
移植方法是捕获SIGCHLD信号，并调用wait或waitpid。
从子进程终止SIGCHLD信号递交时，父进程阻塞于accept调用，然后SIGCHLD的
处理函数会执行，其wait调用取到子进程的PID和终止状态，最后返回。既然
该信号实在父进程阻塞于慢系统调用（accept）时由父进程捕获的，内核会使
accept返回一个EINTR错误（被中断的系统调用）。
我们用术语慢系统调用（slow system call）描述accept函数，该术语也适用
于那些可能永远阻塞的系统调用。举例来说，如果没有客户连接到服务器上，
那么服务器的accept就没有返回的机会。类似的如果客户没有发送数据给服务
器，那么服务器的read调用也将永不返回。适用于慢系统调用的基本规则是：
当阻塞于某个慢系统调用的一个进程捕获某个信号且相应相应信号处理函数返回
时，该系统调用可能返回一个EINTR错误。有些内核自动重启某些被中断的系统
调用。不过为了便于移植，当我们编写捕获信号的程序时，必须对慢系统调用
返回EINTR有所准备。移植性问题是由早期使用的修饰词“可能”、“有些”和对
POSIX的SA_RESTART标志的支持是可选的这一事实造成的。即使某个实现支持
SA_RESTART标志，也并非所有被中断的系统调用都可以自动重启。例如大多数
源自Berkeley的实现从不自动重启select，其中有些实现从不重启accept和
recvfrom。
为了处理被中断的accept，需要判断错误是否是EINTR，如果是则重新调用accept
函数，而不是直接错误返回。对于accept以及诸如read、write、select和open
之类的函数来说，这是合适的。不过有一个函数我们不能重启：connect。如果该函数
返回EINTR，我们就不能再次调用它，否则立即返回一个错误。当connect被一个捕获
的信号中断而且不自动重启时（TCPv2第466页），我们必须调用select来等待连接
完成。如16.3节所述。
 ** 服务器进程终止 **
当服务器进程终止时会关闭所有打开的文件描述符，这回导致向客户端发送一个FIN，
而客户端TCP响应一个ACK，这就是TCP连接终止工作的前半部分。如果此时客户不理会
读取数据时返回的错误，反而写入更多的数据到服务器上会发生什么呢？这种情况是
可能发生的，例如客户可能在读回任何数据之前执行两次针对服务器的写操作，而第
一次写引发了RST。适用于此的规则是：当一个进程向某个已收到RST的套接字执行写
操作时，内核会向该进程发送一个SIGPIPE信号。该信号的默认行为是终止进程，因此
进程必须捕获以免不情愿的被终止。不论该进程是捕获该信号并从其信号处理函数返回，
还是简单地忽略该信号，写操作都将返回EPIPE错误。
一个在Usenet上经常问及的问题是如何在第一次写操作时而不是在第二次写操作时捕获
该信号。这是不可能的，第一次写操作引发RST，第二次写引发SIGPIPE信号。因为写一个
已接收了FIN的套接字不成问题，但写一个已接收了RST的套接字是一个错误。
处理SIGPIPE的建议方法取决于它发生时应用进程想做什么。如果没有特殊的事情要做，
那么将信号处理方法直接设置为SIG_IGN，并假设后续的输出操作将检查EPIPE错误并终止。
如果信号出现时需采取特殊措施（可能需要在日志文件中记录），那么就必须捕获该信号，
以便在信号处理函数中执行所有期望的动作。但是必须意识到，如果使用了多个套接字，
该信号的递交无法告诉我们是哪个套接字出的错。如果我们确实需要知道是哪个write
出错，那么必须不理会该信号，那么从信号处理函数返回后再处理EPIPE错误。
*/
typedef void (*ccsigfunc)(int);
ccsigfunc ccsigact(int sig, ccsigfunc func) {
  /** sigaction - examine and change a signal action **
  #include <signal.h>
  int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);
  It is used to change the action taken by a process on receipt of a specific signal.
  signum specifies the signal and can be any valid signal except SIGKILL and SIGSTOP.
  If act is non-null, the new action for signum is installed from act. If oldact is
  non-null, the previous action is saved in oldact. The sigaction structure:
  struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
  };
  On some architectures a union is involved, don't assign to both sa_handler and
  sa_sigaction. The sa_restorer is not intended for application use (POSIX does
  not specify a sa_restorer field).
  sa_handler specifies the action to be associated with signum and may be SIG_DFL
  for the default action, SIG_IGN to ignore this signal, or pointer to a signal
  handling function. This function receives the signal number as its only argument.
  sa_mask specifies a mask of signals which should be blocked during execution of
  the signal handler. In addition, the signal which triggered the handler will be
  blocked, unless the SA_NODEFER flag is used.
  If SA_SIGINFO is specified in sa_flags, then sa_sigaction (instead of sa_handler)
  specifies the signal-handling function for signum. Other flags can be:
  SA_NOCLDSTOP - If signum is SIGCHLD, don't receive notification when child process
  stop (i.e., when they receive one of SIGSTOP, SIGTSTP, SIGTTIN, or SIGTTOU) or
  resume (i.e., they receive SIGCONT). This flag is only for SIGCHLD.
  SA_NOCLDWAIT (since Linux 2.6) - If signum is SIGHLD, do not transform children
  into zombies when they terminate. This flag is menaingful only when establishing a
  handler for SIGCHLD, or when setting that signal's disposition to SIG_DFL.
  SA_NODEFER - Don't prevent the signal from being received from within its own
  signal handler. This flag is meaningful only when establishing a single handler.
  SA_NOMASK is an obsolete, nonstandard synonym for this flag.
  SA_ONSTACK - Call the signal handler on an alternate signal stack provided by
  sigaltstack(2). If an alternate stack is not available, the default stack will be
  used. This flag is meaningful only when establishing a signal handler.
  SA_RESETHAND - Restore the signal action to the default upon entry to the signal
  handler. This flag is meaningful only when establishing a signal handler. SA_ONESHOT
  is an obsolete, nonstandard synonym for this flag.
  SA_RESTART - Provide behavior compatible with BSD signal semantics by making certain
  ssystem calls restartable across signals. This flag is meaningful only when establishing
  a signal handler. See signal(7) for a discussion of system call restarting.
  SA_RESTORER - Not intended for application use. This flag is used by C libraries to
  indicate that the sa_restorer field contains the address of a "signal trampoline".
  SA_SIGINFO (since Linux 2.2) - The signal handler takes three arguments, not one. In
  this case, sa_sigaction should be set instead of sa_handler. This flag is meaningful
  only when establishing a signal handler.
  It is returns 0 on success, on error, -1 is returned, and errno is set.
   ** POSIX signal set operations **
  #include <signal.h>
  int sigemptyset(sigset_t* set); // set empty
  int sigfillset(sigset_t* set); // set full, including all signals
  int sigaddset(sigset_t* set, int sig);
  int sigdelset(sigset_t* set, int sig);
  int sigismember(const sigset_t* set, int sig);
  Return 0 on success and -1 on error, and sigismember returns 1 if is a member, returns
  0 if not a member, and -1 on error. On error, the errno is set.
  When creating a filled signal set, the glibc sigfillset() function doesn't include
  the two real-time signals used internally by the NPTL threading implementation. See
  nptl(7) for details.
  */
  struct sigaction act, oldact;
  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  /* SA_RESTART标志可可选的。如果设置，由相应信号中断的系统调用将由内核自动重启。
  如果被捕获的信号不是SIGALRM且SA_RESTART有定义，我们就设置标志。对SIGALRM进行特殊
  处理的原因在于，产生该信号的目的正如14.2节讨论的那样，通常是为I/O操作设置超时，
  这种情况下我们希望受阻塞的系统调用被该信号中端掉从内核返回到用户进程。一些较早的
  系统（如SunOS 4.x）默认设置为自动重启被中断的系统调用（内核自动重启系统调用不会返
  回用户进程），并定义了与SA_RESTART互补的SA_INTERRUPT标志。如果定义了该标志，我们
  就在被捕获的信号是SIGALRM时设置它。
  */
  if (sig == SIGALRM) {
  #ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT; /* SunOS 4.x */
  #endif
  } else {
  #ifdef SA_RESTART
    act.sa_flags |= SA_RESTART; /* SVR4, 4.4BSD */
  #endif
  }
  if (sigaction(sig, &act, &oldact) != 0) {
    ccloge("sigaction %s", strerror(errno));
    return SIG_ERR;
  }
  return oldact.sa_handler;
}

void ccsigign(int sig) {
  ccsigact(sig, SIG_IGN);
}

/** accept - accept a connection on a socket **
#include <sys/types.h>
#include <sys/socket.h>
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
It is used with connection-based socket types (SOCK_STREAM,
SOCK_SEQPACKET). It extracts the first connection request on the
queue of pending connections for the listening sockfd, and creates
a new connected socket, and returns a new fd referring to it. The
newly created socket is not in the listening state. The original
sockfd is unaffected by this call.
The addr is a pointer to a sockaddr structure. This structure is
filled in with the address of the peer socket, as known to the
communications layer. The exact format of the address returned is
determined by the socket's address family. When addr is NULL, nothing
is filled in; in this case, addrlen is not used, and should also
be NULL. The addrlen argument is a valud-result argument: the caller
must initialize it to contain the size in bytes of the structure
pointed to by addr; on return it will contain the acutal size of
the peer address. The returned address is truncated if the buffer
provided is too small; in this case, addrlen will return a value
greater than was supplied to the call.
If no pending connections are present on the queue, and the socket
is not marked as nonblocking, accept() blocks the caller until a
connection is present. If the socket is marked nonblocking and no
pending connections are present on the queue, accept() fails with
the error EAGIN or EWOULDBLOCK.
当有一个已完成的连接准备好被accept时，如果使用select会将其作为可读描述符
返回该连接的监听套接字。因此如果我们使用select在某个监听套接字上等待一个
外来连接，那就没必要把该监听套接字设置为非阻塞的，这是因为如果select告诉我
们该套接字上已有连接就绪，那么随后的accept调用不应该阻塞。不幸的是，这里存在
一个可能让我们掉入陷阱的定时问题(Gierth 1996）。为了查看这个问题，我们使TCP
客户端建立连接后发送一个RST到服务器。然后在服务器端当select返回监听套接字
可读后睡一段时间，然后才调用accept来模拟一个繁忙的服务器。此时客户在服务器调
用accept之前就中止了连接时，源自Berkeley的实现不把这个中止的连接返回给服务器
进程，而其他实现应该返回ECONNABORTED错误，却往往代之以返回EPROTO错误。考虑
源自Berkeley的实现，在TCP收到RST后会将已完成的连接移除出队列，假设队列中没有
其他已完成的连接，此时调用accept时会被阻塞，直到其他某个客户建立一个连接为止。
但是在此期间服务器单纯阻塞在accept调用上，无法处理任何其他以就绪的描述符。本
问题的解决方法是：当使用select获悉某个监听套接字上有已完成的连接准备好时，总
是把这个监听套接字设为非阻塞；在后续的accept调用中忽略以下错误继续执行直到队
列为空返回EAGIN或EWOULDBLOCK为止：ECONNABORTED（POSIX实现，客户中止了连接）、
EPROTO(SVR4实现，客户中止了连接）和EINTR（有信号被捕获）。
以上在accept之前收到RST的情况，不同实现可能以不同的方式处理这种中止的连接。
源自Berkeley的实现完全在内核中处理中止的连接，服务器进程根本看不到。然而大多数
SVR4实现返回一个错误给服务其进程作为accept的返回结果，不过错误本身取决于实现。
这些SVR4实现返回一个EPROTO，而POSIX指出返回的必须是ECONNABORTED（software
caused connection abort）。POSIX作出修正的理由在于，流子系统（streams subsystem）
中发生某些致命的协议相关事件时，也会返回EPROTO。要是对于客户引起的一个已建立连接
的非致命中止也返回同样的错误，那么服务器就不知道该再次调用accept还是不该。换成
是ECONNABORTED错误，服务器可以忽略它，再次调用accept即可。源自Berkeley的内核从
不把该错误传递给进程的做法所涉及的步骤在TCPv2中得到阐述，引发该错误的RST在第964
页到达处理，导致tcp_close被调用。
*/
void ccsockaccept(int listenfd) {
  union llsockaddr rdaddr;
  socklen_t addrlen = sizeof(union llsockaddr);

  struct ccsockconn conn;
  int n = 0;

  cczero(&rdaddr, addrlen);
  ccscokconninit(&conn);

  for (; ;) {
    /** accept - accept a connection on a socket **
    int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
    on success, it returns a nonnegative interger that is a fd for the accepted
    socket. on error, -1 is returned, and errno is set appropriately.
    Error handling - linux accept passes already-pending network errors on the
    new socket as an error code from accept(). this behavior differs from other
    BSD socket implementations. for reliable operation the application should
    detect the network errors defined for the protocol after accept() and treat
    them like EAGAIN by retrying. in the case of TCP/IP, these are ENETDOWN,
    EPROTO, ENOPROTOOPT, EHOSTDOWN, ENONET, EHOSTUNREACH, EOPNOTSUPP, and
    ENETUNREACH.
    */
    if ((conn->connfd = accept(listenfd, &rdaddr.sa, &addrlen)) != -1) {
      ccsockntop(&rdaddr, addrlen, &conn->remoteip, &conn->remoteport);
      cclocaladdr(conn->connfd, &conn->localip, &conn->localport);
      ccdispatchconn(&conn);
      continue;
    }

    n = errno;
    if (n == EAGAIN || n == EWOULDBLOCK) {
      /* no more pending completed connections in the kernel */
      return;
    }

    switch (n) {
    case EINTR: /* system call was interrupted by a signal */
    case ECONNABORTED: /* a connection has been aborted */
    case EPROTO: /* protocol error */
      /* try to call accept again */
      break;
    case EBADF: /* sockfd is not an open fd */
    case EFAULT: /* the addr is not in a writable part of the user address space */
    case EINVAL: /* sockfd is not listening for connections, or addrlen is invalid */
    case EMFILE: /* per-process limit on the number of open fds reached */
    case ENFILE: /* system-wide limit on the total number of open files reached */
    /* no enough free memory. this often means that the memory allocation is limited */
    case ENOMEM: case ENOBUFS: /* by the socket buffer limits, not by the system memroy */
    case ENOTSOCK: /* the sockfd does not refer to a socket */
    case EOPNOTSUPP: /* the referenced socket is not of type SOCK_STREAM */
    case EPERM: /* firewall rules forbid connection */
      ccloge("accept %s", strerror(n));
      return; /* unrecoverable error, return */
    default:
      /* in addition, network errors for the new socket and as defined for the protocol
      may be returned. various linux kernels can return other errors such as ENOSR,
      ESOCKTNOSUPPORT, EPROTONOSUPPORT, ETIMEOUT. the value ERESTARTSYS may be seen
      during a trace. */
      ccloge("accept unknown error: %s", strerror(n));
      return;
    }
  }
}

/** TCP连接的各种可能错误 **
服务器主机崩溃 - 为了模拟这种情形必须在不同的主机上运行客户端和服务器。我们先启动
服务器再启动客户，接着在客户上键入一行文本以确认连接工作正常，然后从网络上断开服务
器主机，并在客户上键入另一行文本。这样同时也模拟了当客户发送数据时服务器不可达的情
形（例如建立连接后某些中间路由器不工作）。当服务器崩溃时，已有的网络连接上不会发出
任何东西。这里我们假设的是主机崩溃，而不是执行关机命令。在客户键入文本后，客户TCP
会持续重传数据，试图从服务器上接收到一个ACK。TCPv2的25.11节给出了TCP重传的一个经典
模式：源自Berkeley的实现重传该数据包12次，共等待约9分钟才放弃。当客户TCP最终放弃时
（假设在这段时间内，服务器主机没有重新启动或假设主机仍然不可达），客户进程会返回一个
错误。如果服务器主机已崩溃从而对客户数据没有响应，则返回的错误将是ETIMEOUT。然而如果
某个中间路由器判定服务器主机已不可达，从而响应一个"destination unreachable"的ICMP
消息，那么所返回的错误是EHOSTUNRACH或ENETUNREACH。
尽管我们的客户最终还是会发现对端主机已崩溃或不可达，不过有时候我们需要比不得不等待
9分钟更快的检测出这种情况。所用的方法就是对读操作设置一个超时，我们将在14.2中讨论
这一点。这里讨论的情形只有在我们向服务器发送数据时才能检测到它已经崩溃。如果我们
不主动给它发送数据也想检测服务器主机的崩溃，那么需要另外一个技术，也就是我们将在
7.5节讨论的SO_KEEPALIVE套接字选项，或某些客户/服务器心搏函数。
如果服务器崩溃后又重启，重新连接到网络中。此时假设没有使用SO_KEEPALIVE选项，那么所
发生的事情是：服务器崩溃后客户发送的数据会在重传，当服务器重新启动并连网后，它的
TCP丢失了崩溃前的所有连接信息，因此服务器的TCP会响应一个RST；当客户收到该RST时，客
户正阻塞于读取系统调用，导致该调用返回ECONNRESET错误。
服务器主机关机 - Unix系统关机时，init进程通常先给所有进程发送SIGTERM信号（该信号可
被捕获），等待一段固定的时间（往往在5到20s之间），然后给所有仍在运行的进程发送
SIGKILL信号（该信号不能被捕获）。这么做留给所有运行进程一小段时间来清除和终止。如果
我们忽略SIGTERM信号（如果默认处置SIGTERM也会终止进程），我们的服务器将由SIGKILL信号
终止。进程终止会使所有打开这的描述符都被关闭，然后服务器会发生正常的TCP断连过程。正
如5.12节所讨论的一样，我们必须在客户中使用select或poll函数，以防TCP断连时客户阻塞在
其他的函数中而不能快速知道TCP已经断连了。
*/

_int cctcpconnect(ccfrom remoteip, ushort port, struct ccsockconn* conn) {
  /** connect - initiate a conneciton on a socket **
  #include <sys/types.h>
  #include <sys/socket.h>
  int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
  It connects the socket referred to sockfd to the address specified by addr.
  The addrlen specifies the size of addr. The format of the address in addr
  is determined by the address space of the socket sockfd.
  If the sockfd is of type SOCK_DGRAM, then addr is the address to which
  datagrams are sent by default, and the only address from which datagrams
  are received. If the socket is of type SOCK_STREAM or SOCK_SEQPACKET, this
  call attempts to make a connection to the socket that is bound to the
  address specified by addr.
  Generally, connection-based protocol sockets may successfully connect()
  only once; connectionless protocol sockets may use connect() multiple
  times to change their association. Connectionless sockets may dissolve
  the association by connecting to an address with the sa_family member of
  sockaddr set to AF_UNSPEC (supported on Linux since kernel 2.2).
  If the connection or binding succeeds, zero is returned. On error, -1
  is returned, and errno is set. If connect() fails, consider the state of
  the socket as unspecified. Protable appliations should close the socket
  and create a new one for reconnecting.
  EACCES, EPERM - The user tried to connect to a broadcast address without
  having the socket broadcast flag enbaled or the conneciton request failed
  because of a local firewall rule.
  EADDRINUSE - local address is already in use.
  EADDRNOTAVAIL - (Internet domain sockets) The socket referred to by sockfd
  had not previously been bound to and address and, upon attempting to bind
  it to an ephemeral port, it was determined that all port numbers in the
  ephemeral port range are currently in use. See the discussion of
  /proc/sys/net/ipv4/ip_local_port_range in ip(7).
  EAFNOSUPPORT - the passed address didn't have the correct address family.
  EAGAIN - insufficient entries in the routing cache.
  EALREADY - the socket is nonblocking and a previous connection attempt
  has not yet been completed.
  EBADF - sockfd is not a valid open fd.
  ECONNREFUSHED - no one listening on the remote address.
  EFAULT - the socket structure address is outside the user's address space.
  EINPROGRESS - the socket is nonblocking and the connection cannot be
  completed immediately. It is possible to select or poll for completion
  by selecting the socket for writing. After select indicates writability,
  use getsockopt to read the SO_ERROR option at level SO_SOCKET to
  determine whether connect() completed successfully (SO_ERROR is zero) or
  unsuccessfully.
  EINTR - the system call was interrupted by a signal that was caught.
  EISCONN - the socket is already connected.
  ENETUNREACH - network is unreachable.
  ENOTSOCK - the sockfd does not refer to a socket.
  EPROTOTYPE - the socket type does not support the requested communications
  protocol. This error can occur, for example, on an attempt to conect a
  UNIX domain datagram socket to a stream socket.
  ETIMEDOUT - timeout while attempting connection. the server may be too busy
  to accept new connections. Note that for ip sockets the timeout may be very
  long when syncookies are enabled on the server.
  如果是TCP套接字，调用connect函数会激发TCP三次握手过程，而且仅在连接建立成功
  或出错时才返回，其中错误返回可能有以下几种情况：
  如果TCP客户没有收到SYN的回应，则返回ETIMEOUT错误。举例来说，调用connect()时，
  4.4BSD内核发送一个SYN，若无响应则等待6s后再发送一个，若仍然无响应则等待24s
  后再发送一个（TCPv2第828页）。若总共等了75s后仍未收到响应则返回本错误。
  若对客户的SYN响应的是RST，则表明该服务器主机在我们指定的端口上没有进程在等待
  与之连接（如服务器进程也许没有在运行）。这是一种硬错误（hard error），客户一
  接收到RST就马上返回ECONNREFUSED错误。RST是TCP发生错误时发送的一种TCP数据包。
  产生RST的三个条件是：目的地为某端口的SYN到达，然而该端口上没有正在监听的服务
  器；TCP想取消一个已有连接；TCP接收到一个根本不存在的连接上的数据包。
  若客户发出的SYN在中间某个路由器上引发一个destination unreachable（目的地不可
  达）ICMP错误，则认为是一种软错误（soft error）。客户主机内核保存该消息，并按
  第一种情况中所述的时间间隔重新发送SYN。若在某个规定的时间后仍未收到响应，则把
  保存的消息作为EHOSTUNREADCH或ENETUNREACH错误返回给进程。以下两种情况也是可能
  的：一是按照本地系统的转发表，根本没有到达远程系统的路径；二是connect调用根本
  不等待就返回。许多早期系统比如4.2BSD在收到“目的地不可达”ICMP错误时会不正确地
  放弃建立连接的尝试。注意，网路不可达的错误被认为已过时，应用进程应该把
  ENETUNREACH和EHOSTUNREACH作为相同的错误对待。
  按照TCP状态转换图，connect函数导致当前套接字从CLOSED状态转移到SYN_SENT状态，
  若成功再转移到ESTABLISHED状态，如connect失败则该套接字不再可用，必须关闭。
  我们不能对这样的套接字再次调用connect函数。*/
  union llsockaddr sa;
  int connfd = 0, family = AF_INET;
  socklen_t len = (socklen_t)ccsockpton(remoteip, port, &sa);
  ccstrsetempty(&conn->remoteip);
  ccstrsetempty(&conn->localip);
  conn->localport = conn->remoteport = 0;
  conn->connfd = -1;
  if (len == 0) {
    ccloge("connect invalid ip %s", remoteip.start);
    return EINVAL;
  }
  if (ccstrcontain(remoteip, ':')) {
    family = AF_INET6;
  }
  if ((connfd = socket(family, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    ccloge("socket %s", strerror(errno));
    return errno;
  }
  if (connect(connfd, &sa.sa, len) != 0) {
    cclogw("connect %s", strerror(errno));
    ccsockclose(&connfd);
    return errno;
  }
  conn->connfd = connfd;
  ccstrsetfrom(&conn->remoteip, remoteip);
  conn->remoteport = port;
  cclocaladdr(connfd, &conn->localip, &localport);
  return 0;
}

void ccsockshutdown(int sockfd, char how) {
  /** shutdown - shut down part of a full-duplex connection **
  #include <sys/socket.h>
  int shutdown(int sockfd, int how);
  This call causes all or part of a full-duplex connection on the socket
  associated with sockfd to be shut down. If how is SHUT_RD, further
  receptions will be disallowed. If how is SHUT_WR, further transmissions
  will be disallowed. If how is SHUT_RDWR, further receptions and
  transmissions will be disallowed.
  On success, zero is returned. On error, -1 is returned, and errno is set.
  EBADF - sockfd is not a valid fd.
  EINVAL - an invalid value was specified in how.
  ENOTCONN - the specified socket is not connected.
  ENOTSOCK - the sockfd is not refer to a socket. */
  int flag = (how == 'r' ? SHUT_RD : (how == 'w' ? SHUT_WR : SHUT_RDWR));
  if (how != 'r' && how != 'w' && how != 'a') {
    ccloge("shutdown invalid value");
    return;
  }
  if (shutdown(sockfd, flag) != 0) {
    ccloge("shutdown %s", strerror(errno));
  }
}

void ccsockclose(int* sockfd) {
  /** close - close a file descriptor **
  #include <unistd.h>
  int close (int fd);
  It closes a fd, so that it no longer refers to any file and may be reused.
  Any record locks held on the file it was associated with, and owned by the
  process, are removed (regardless of the fd that was used to obtain the lock).
  If fd is the last fd referring to the underlying open fd, the resources
  associated with the open fd are freed.
  It returns zero on success. On error, -1 is returned, and errno is set.
  EBADF - fd isn't a valid open fd.
  EINTR - the close() call was interrupted by a signal.
  EIO - an I/O error occurred.
  close() should not be retried after an error. Retrying the close() after a
  failure return is the wrong thing to do, since this may cause a reused fd
  from another thread to be closed. This can occur because the Linux kernel
  always releases the fd early in the close operation, freeing it for resue.
  关闭一个TCP套接字的默认行为是把该套接字标记为已关闭，然后立即返回到调用进程。
  该套接字不能再由调用进程使用，也就是说不能再作为read或write的第一个参数。然而
  TCP将尝试发送已排队等待发送到对端的任何数据，发送完毕后发生正常的TCP断连操作。
  我们将在7.5节介绍的SO_LINGER套接字选项可以用来改变TCP套接字的这种默认行为。我
  们将在那里介绍TCP应用进程必须怎么做才能确信对端应用进程已收到所有排队数据。
  关闭套接字只是导致相应描述符的引用计数减1，如果引用计数仍大于0，这个close调用
  并不会引发TCP断连流程。如果我们确实想在某个TCP连接上发送一个FIN，可以调用
  shutdown函数以代替close()，我们将在6.5节阐述这么做的动机。*/
  if (*sockfd != -1) {
    if (close(*sockfd) != 0) {
      cclogw("close sockfd %d", strerror(errno));
    }
    *sockfd = -1;
  }
}

_int cctcpserverbind(struct cctcpserver* s, struct ccfrom localip, ushort port) {
  int sockfd = 0, family = AF_INET;
  s->listenfd = -1;
  if (ccstrcontain(localip, ':')) {
    family = AF_INET6;
  }
  if ((sockfd = socket(family, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    ccloge("socket %s", strerror(errno));
    return errno;
  }
  if (!ccsockbind(sockfd, localip, port)) {
    ccsockclose(&sockfd);
    return EINVAL;
  }
  s->listenfd = sockfd;
  return 0;
}

void ccplattest() {
  ccassert(sizeof(struct ccmutex) >= sizeof(pthread_mutex_t));
  ccassert(sizeof(struct ccrwlock) >= sizeof(pthread_rwlock_t));
  ccassert(sizeof(struct cccondv) >= sizeof(pthread_cond_t));
  cclogd("pthread_t %s-byte", ccutos(sizeof(pthread_t)));
  cclogd("pthread_mutext_t %s-byte", ccutos(sizeof(pthread_mutex_t)));
  cclogd("pthread_rwlock_t %s-byte", ccutos(sizeof(pthread_rwlock_t)));
  cclogd("INET_ADDRSTRLEN ipv4 string max len %s", ccitos(INET_ADDRSTRLEN));
  cclogd("INET6_ADDRSTRLEN ipv6 string max len %s", ccitos(INET6_ADDRSTRLEN));
  cclogd("struct sockaddr_in %s-byte", ccitos(sizeof(struct sockaddr_in)));
  cclogd("struct sockaddr_in6 %s-byte", ccitos(sizeof(struct sockaddr_in6)));
}
