#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
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

#if defined(CC_OS_APPLE)
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
    return (struct cctime){0};
  }
  return (struct cctime){(sright_int)tv.tv_sec, (umedit_int)tv.tv_usec*1000};
}
#else
static struct cctime llgettime(clockid_t id) {
  struct timespec spec = {0};
  if (clock_gettime(id, &spec) != 0) {
    ccloge("clock_gettime %d %s", id, strerror(errno));
    return (struct cctime){0};
  }
  return (struct cctime){(sright_int)spec.tv_sec, (umedit_int)spec.tv_nsec};
}

struct cctime ccthreadtime() {
  return llgettime(CLOCK_THREAD_CPUTIME_ID);
}

struct cctime ccprocesstime() {
  return llgettime(CLOCK_PROCESS_CPUTIME_ID);
}

static sright_int llgetres(clockid_t id) {
  struct timespec spec = {0};
  if (clock_getres(id, &spec) != 0) {
    ccloge("clock_getres %s", strerror(errno));
    return 0;
  }
  return ((sright_int)spec.tv_sec)*1000000000LL + (sright_int)spec.tv_nsec;
}

sright_int ccgetsres() {
  return llgetres(CLOCK_REALTIME);
}

sright_int ccgetbres() {
  clockid_t id = CLOCK_MONOTONIC;
#ifdef CC_OS_LINUX
  id = CLOCK_BOOTTIME;
#endif
  return llgetres(id);
}

sright_int ccgettres() {
  return llgetres(CLOCK_THREAD_CPUTIME_ID);
}

sright_int ccgetpres() {
  return llgetres(CLOCK_PROCESS_CPUTIME_ID);
}
#endif

struct cctime ccsystime() {
#if defined(CC_OS_APPLE)
  return llsystime();
#else
  return llgettime(CLOCK_REALTIME);
#endif
}

static void llsetyear(struct ccdate* date, sright_int year, sright_int yday) {
  uoctet_int sign = 0;
  if (year < 0) { year = -year; sign = 2; }
  if (yday < 1 || yday > 366) {
    yday = (yday < 1 ? 1 : 366);
    ccloge("tm_yday invalid %s", ccitos(yday));
  }
  date->yearlow = (umedit_int)(year & 0xFFFFFFFF);
  date->ydaylow = (uoctet_int)(yday & 0xFF);
  date->high = (((uoctet_int)((year >> 30) & 0xFC)) | sign | ((uoctet_int)((yday >> 8) & 0x01)));
}

static struct ccdate llgetdate(time_t secs) {
  struct tm st;
  struct ccdate date = {0};
  cczeron(&st, sizeof(struct tm));
  /* struct tm* gmtime_r(const time_t* secs, struct tm* out);
  The function converts the time secs to broken-down time representation,
  expressed in Coordinated Universal Time (UTC), i.e. since Epoch. */
  if (gmtime_r(&secs, &st) != &st) { /* gmtime_r needs _POSIX_C_SOURCE >= 1 */
    ccloge("gmtime_r %s", strerror(errno));
    return date;
  }
  /* tm_year - number of years since 1900, tm_yday - from 0 to 365 */
  llsetyear(&date, st.tm_year + 1900, st.tm_yday + 1);
  date.month = (soctet_int)(st.tm_mon + 1); /* tm_mon - from 0 to 11 */
  date.wday = (soctet_int)st.tm_wday; /* tm_wday - from 0 to 6, 0 is Sunday */
  /* in many implementations, including glibc, a 0 in tm_mday is
  interpreted as meaning the last day of the preceding month.
  这里的意思是，在将分解结构tm转换成time_t时（例如mktime），如果将
  tm_mday设为0则表示当前天数是前一个月的最后一天 */
  date.day = (soctet_int)st.tm_mday; /* tm_mday - from 1 to 31 */
  date.hour = (soctet_int)st.tm_hour; /* tm_hour - from 0 to 23 */
  date.min = (soctet_int)st.tm_min; /* tm_min - from 0 to 59 */
  date.sec = (soctet_int)st.tm_sec; /* tm_sec - from 0 to 60, 60 can be leap second */
  /* Single UNIX Specification 的以前版本允许双闰秒，于是tm_sec值的有效范围是
  0到61。但是UTC的正式定义不允许双闰秒，所以现在tm_sec值的有效范围是0到60。*/
  if (st.tm_isdst > 0) {
    /* daylight saving time is in effect. gmtime_r将时间转换成协调统一时间的分解
    结构，不会像localtime_r那样考虑本地时区和夏令时，该值应该在此处无效 */
    cclogw("gmtime_r invalid tm_isdst");
  }
  return date;
}

sright_int ccgetyear(struct ccdate* date) {
  sright_int year = (((sright_int)(date->high >> 2)) << 32) | ((sright_int)date->yearlow);
  if (date->high & 0x02) { year = -year; }
  return year;
}

sright_int ccgetyday(struct ccdate* date) {
  return (((sright_int)(date->high & 0x01)) << 8) | ((sright_int)date->ydaylow);
}

struct ccdate ccdatefromi(sright_int utcsecs) {
  return llgetdate(utcsecs);
}

struct ccdate ccdatefrom(struct cctime utc) {
  struct ccdate date = llgetdate(utc.sec);
  date.nsec = utc.nsec;
  return date;
}

struct ccdate ccgetdate() {
  return ccdatefrom(ccsystime());
}

struct cctime ccinctime() {
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

/** File and attribute */

sright_int ccfilesize(struct ccfrom name) {
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
  if (name.start >= name.beyond) return 0;
  cczeron(&st, sizeof(struct stat));
  if (lstat((const char*)name.start, &st) != 0) {
    ccloge("lstat %s %s", strerror(errno), name.start);
    return 0;
  }
  return (sright_int)st.st_size;
}

struct ccfileattr ccgetfileattr(struct ccfrom name) {
  struct stat st;
  struct ccfileattr fa = {0};
  cczeron(&st, sizeof(struct stat));
  if (name.start >= name.beyond) return fa;
  if (lstat((const char*)name.start, &st) != 0) {
    ccloge("lstat %s %s", strerror(errno), name.start);
    return fa;
  } /* the time stored in stat is measured in seconds since 00:00:00 UTC, January 1, 1970 */
  fa.fsize = (sright_int)st.st_size;
  fa.ctime = (sright_int)st.st_ctime;
  fa.atime = (sright_int)st.st_atime;
  fa.mtime = (sright_int)st.st_mtime;
  fa.gid = (sright_int)st.st_gid;
  fa.uid = (sright_int)st.st_uid;
  fa.mode = (sright_int)st.st_mode;
  fa.isfile = S_ISREG(st.st_mode);
  fa.isdir = S_ISDIR(st.st_mode);
  fa.islink = S_ISLNK(st.st_mode);
  return fa;
}

/** Directory stream **/

struct ccdirstream ccopendir(struct ccfrom name) {
  /** opendir **
  #include <sys/types.h>
  #include <dirent.h>
  DIR* opendir(const char* name);
  It opens a directory stream corresponding to the name, and returns
  a pointer to the directory stream. The stream is positioned at the
  first entry in the directory. Filename entries can be read from a
  directory stream using readdir(3). */
  struct ccdirstream d = {0};
  if (name.start >= name.beyond) return d;
  if ((d.stream = opendir((const char*)name.start)) == 0) {
    ccloge("opendir %s", strerror(errno));
    return d;
  }
  return d;
}

void ccclosedir(struct ccdirstream* d) {
  if (d->stream) {
    if (closedir((DIR*)d->stream) != 0) {
      ccloge("closedir %s", strerror(errno));
    }
    d->stream = 0;
  }
}

struct ccfrom ccreaddir(struct ccdirstream* d) {
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
  s.start = (uoctet_int*)entry->d_name;
  s.beyond = s.start + strlen(entry->d_name);
  return s;
}

/** Thread and synchronization **/

/**
 * thread-specific data key
 */

nauty_bool ccthrkey_init(struct ccthrkey* self) {
  /** pthread_key_create - thread-specific data key creation **
  #include <pthread.h>
  int pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
  If successful, it shall store the newly created key value at *key and
  shall return zero. Otherwise, an error number shall be returned to
  indicate the error. */
  int n = pthread_key_create((pthread_key_t*)self, 0);
  if (n != 0) {
    ccloge("pthread_key_create %s", strerror(n));
    return false;
  }
  return true;
}

void ccthrkey_free(struct ccthrkey* self) {
  /** pthread_key_delete - thread-specific data key deletion **
  #include <pthread.h>
  int pthread_key_delete(pthread_key_t key); */
  int n = pthread_key_delete(*(pthread_key_t*)self);
  if (n != 0) {
    ccloge("pthread_key_delete %s", strerror(n));
  }
}

void* ccthrkey_getdata(struct ccthrkey* self) {
  /** thread-specific data management **
  #include <pthread.h>
  void* pthread_getspecific(pthread_key_t key);
  int pthread_setspecific(pthread_key_t key, const void* value);
  No errors are returned from pthread_getspecific(), pthread_setspecific() may fail:
  ENOMEM - insufficient memory exists to associate the value with the key.
  EINVAL - the key value is invalid. */
  return pthread_getspecific(*(pthread_key_t*)self);
}

nauty_bool ccthrkey_setdata(struct ccthrkey* self, const void* data) {
  /* different threads may bind different values to the same key, the value
  is typically a pointer to blocks of dynamically allocated memory that have
  been reserved for use by the calling thread. */
  int n = pthread_setspecific(*(pthread_key_t*)self, data);
  if (n != 0) {
    ccloge("pthread_setspecific %s", strerror(n));
    return false;
  }
  return true;
}

/**
 * mutex
 */

nauty_bool ccmutex_init(struct ccmutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_init(mutex, 0);
  if (n != 0) {
    ccloge("pthread_mutex_init %s", strerror(n));
    return false;
  }
  return true;
}

void ccmutex_free(struct ccmutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_destroy(mutex);
  if (n != 0) {
    ccloge("pthread_mutex_destroy %s", strerror(n));
  }
}

nauty_bool ccmutex_lock(struct ccmutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_lock(mutex);
  if (n != 0) {
    ccloge("pthread_mutex_lock %s", strerror(n));
    return false;
  }
  return true;
}

void ccmutex_unlock(struct ccmutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_unlock(mutex);
  if (n != 0) {
    ccloge("pthread_mutex_unlock %s", strerror(n));
  }
}

nauty_bool ccmutex_trylock(struct ccmutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_trylock(mutex);
  if (n == 0) {
    return true;
  }
  if (n != EBUSY) {
    ccloge("pthread_mutex_trylock %s", strerror(n));
  }
  return false;
}

/**
 * read/write lock
 */

nauty_bool ccrwlock_init(struct ccrwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_init(lock, 0);
  if (n != 0) {
    ccloge("pthread_rwlock_init %s", strerror(n));
    return false;
  }
  return true;
}

void ccrwlock_free(struct ccrwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_destroy(lock);
  if (n != 0) {
    ccloge("pthread_rwlock_destroy %s", strerror(n));
  }
}

nauty_bool ccrwlock_read(struct ccrwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_rdlock(lock);
  if (n != 0) {
    ccloge("pthread_rwlock_rdlock %s", strerror(n));
    return false;
  }
  return true;
}

nauty_bool ccrwlock_write(struct ccrwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_wrlock(lock);
  if (n != 0) {
    ccloge("pthread_rwlock_wrlock %s", strerror(n));
    return false;
  }
  return true;
}

nauty_bool ccrwlock_tryread(struct ccrwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_tryrdlock(lock);
  if (n != 0) {
    ccloge("pthread_rwlock_tryrdlock %s", strerror(n));
    return false;
  }
  return true;
}

nauty_bool ccrwlock_trywrite(struct ccrwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_trywrlock(lock);
  if (n != 0) {
    ccloge("pthread_rwlock_trywrlock %s", strerror(n));
    return false;
  }
  return true;
}

void ccrwlock_unlock(struct ccrwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_unlock(lock);
  if (n != 0) {
    ccloge("pthread_rwlock_unlock %s", strerror(n));
  }
}

/**
 * condition variable
 */

nauty_bool cccondv_init(struct cccondv* self) {
  pthread_cond_t* cond = (pthread_cond_t*)self;
  int n = pthread_cond_init(cond, 0);
  if (n != 0) {
    ccloge("pthread_cond_init %s", strerror(n));
    return false;
  }
  return true;
}

void cccondv_free(struct cccondv* self) {
  pthread_cond_t* cond = (pthread_cond_t*)self;
  int n = pthread_cond_destroy(cond);
  if (n != 0) {
    ccloge("pthread_cond_destroy %s", strerror(n));
  }
}

nauty_bool cccondv_wait(struct cccondv* self, struct ccmutex* mutex) {
  pthread_cond_t* c = (pthread_cond_t*)self;
  pthread_mutex_t* m = (pthread_mutex_t*)mutex;
  int n = pthread_cond_wait(c, m);
  if (n != 0) {
    ccloge("pthread_cond_wait %s", strerror(n));
    return false;
  }
  return true;
}

nauty_bool cccondv_timedwait(struct cccondv* self, struct ccmutex* mutex, struct cctime time) {
  pthread_cond_t* c = (pthread_cond_t*)self;
  pthread_mutex_t* m = (pthread_mutex_t*)mutex;
  struct timespec tm;
  tm.tv_sec = (time_t)time.sec;
  tm.tv_nsec = (long)time.nsec;
  int n = pthread_cond_timedwait(c, m, &tm);
  if (n == 0) {
    return true;
  }
  if (n != ETIMEDOUT) {
    ccloge("pthread_cond_timedwait %d", strerror(n));
    return false;
  }
  return true;
}

void cccondv_signal(struct cccondv* self) {
  pthread_cond_t* cond = (pthread_cond_t*)self;
  int n = pthread_cond_signal(cond);
  if (n != 0) {
    ccloge("pthread_cond_signal %s", strerror(n));
  }
}

void cccondv_broadcast(struct cccondv* self) {
  pthread_cond_t* cond = (pthread_cond_t*)self;
  int n = pthread_cond_broadcast(cond);
  if (n != 0) {
    ccloge("pthread_cond_broadcast %s", strerror(n));
  }
}

/**
 * thread
 */

struct ccthrid ccplat_selfthread() {
  struct ccthrid thrid;
  *((pthread_t*)&thrid) = pthread_self();
  return thrid;
}

nauty_bool ccplat_createthread(struct ccthrid* thrid, void* (*start)(void*), void* para) {
  pthread_t* thread = (pthread_t*)thrid;
  int n = pthread_create(thread, 0, start, para);
  if (n != 0) {
    ccloge("pthread_create %s", strerror(n));
    return false;
  }
  return true;
}

void ccplat_threadsleep(uright_int us) {
  struct timespec req;
  req.tv_sec = (time_t)(us/1000000);
  req.tv_nsec = (long)(us%1000000*1000);
  if (nanosleep(&req, 0) != 0) {
    if (errno != EINTR) {
      ccloge("nanosleep %s", strerror(errno));
    }
  }
}

void ccplat_threadexit() {
  pthread_exit((void*)1);
}

int ccplat_threadjoin(struct ccthrid* thrid) {
  int n = 0;
  void* exitcode = 0;
  pthread_t* thread = (pthread_t*)thrid;
  /* wait thread terminate, if already terminated then return
  immediately. the thread needs to be joinable. join a alreay
  joined thread will results in undefined behavior. */
  if ((n = pthread_join(*thread, &exitcode)) != 0) {
    ccloge("pthread_join %s", strerror(n));
  }
  return (int)(signed_ptr)exitcode;
}

/** Linux core test **/

void ccplattest() {
  ccassert(sizeof(struct ccmutex) >= sizeof(pthread_mutex_t));
  ccassert(sizeof(struct ccrwlock) >= sizeof(pthread_rwlock_t));
  ccassert(sizeof(struct cccondv) >= sizeof(pthread_cond_t));
  ccassert(sizeof(int) <= sizeof(umedit_int)); /* test file descriptor size */
  cclogd("pthread_t %s-byte", ccutos(sizeof(pthread_t)));
  cclogd("pthread_mutext_t %s-byte", ccutos(sizeof(pthread_mutex_t)));
  cclogd("pthread_rwlock_t %s-byte", ccutos(sizeof(pthread_rwlock_t)));
}

