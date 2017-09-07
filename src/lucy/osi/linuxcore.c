#include "osi/linuxpref.h"
#include "lucycore.h"

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

#if defined(L_PLAT_APPLE)
static l_time llsystemtime() {
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
  l_time time = {0};
  struct timeval tv = {0};

  if (gettimeofday(&tv, 0) != 0) {
    l_loge_1("gettimeofday %s", lserror(errno));
    return time;
  }

  time.sec = l_cast(l_long, tv.tv_sec);
  time.nsec = l_cast(l_umedit, tv.tv_usec * 1000);
  return time;
}

#else

static l_time llgettime(clockid_t id) {
  l_time time = {0};
  struct timespec spec = {0};

  if (clock_gettime(id, &spec) != 0) {
    l_loge_2("clock_gettime %d %s", ld(id), lserror(errno));
    return time;
  }

  time.sec = l_cast(l_long, spec.tv_sec);
  time.nsec = l_cast(l_umedit, spec.tv_nsec);
  return time;
}

l_time l_thread_time() {
  return llgettime(CLOCK_THREAD_CPUTIME_ID);
}

l_time l_process_time() {
  return llgettime(CLOCK_PROCESS_CPUTIME_ID);
}

static l_long llgetres(clockid_t id) {
  struct timespec spec = {0};
  if (clock_getres(id, &spec) != 0) {
    l_loge_1("clock_getres %s", lserror(errno));
    return 0;
  }
  return ((l_long)spec.tv_sec)*1000000000LL + (l_long)spec.tv_nsec;
}

l_long l_system_time_res() {
  return llgetres(CLOCK_REALTIME);
}

l_long l_monotonic_time_res() {
  clockid_t id = CLOCK_MONOTONIC;
#ifdef L_PLAT_LINUX
  id = CLOCK_BOOTTIME;
#endif
  return llgetres(id);
}

l_long l_thread_time_res() {
  return llgetres(CLOCK_THREAD_CPUTIME_ID);
}

l_long l_process_time_res() {
  return llgetres(CLOCK_PROCESS_CPUTIME_ID);
}
#endif

l_time l_system_time() {
#if defined(L_PLAT_APPLE)
  return llsystemtime();
#else
  return llgettime(CLOCK_REALTIME);
#endif
}

l_time l_monotonic_time() {
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
#ifdef L_PLAT_APPLE
  return llsystemtime();
#else
  clockid_t id = CLOCK_MONOTONIC;
#ifdef L_PLAT_LINUX
  id = CLOCK_BOOTTIME;
#endif
  return llgettime(id);
#endif
}

static void llsetyear(l_date* date, l_long year) {
  if (year > l_max_umedit) {
    date->year = l_cast(l_umedit, year & 0xffffffff);
    date->high = l_cast(l_byte, (year & 0xff00000000) >> 32);
  } else {
    date->year = l_cast(l_umedit, year & 0xffffffff);
    date->high = 0;
  }
}

static void llsetwdaymonth(l_date* date, int wday, int month) {
  date->wdmon = (l_cast(l_byte, wday & 0x0f) << 4) | l_cast(l_byte, month & 0x0f);
}

static l_date llgetdate(time_t secs) {
  struct tm st;
  l_date date = {0};
  l_zero_n(&st, sizeof(struct tm));
  /* struct tm* gmtime_r(const time_t* secs, struct tm* out);
  The function converts the time secs to broken-down time representation,
  expressed in Coordinated Universal Time (UTC), i.e. since Epoch. */
  if (gmtime_r(&secs, &st) != &st) { /* gmtime_r needs _POSIX_C_SOURCE >= 1 */
    l_loge_1("gmtime_r %s", lserror(errno));
    return date;
  }
  /* tm_year - number of years since 1900 */
  llsetyear(&date, st.tm_year + 1900);
  /* tm_wday - from 0 to 6, 0 is Sunday, tm_mon - from 0 to 11 */
  llsetwdaymonth(&date, st.tm_wday, st.tm_mon + 1);
  /* in many implementations, including glibc, a 0 in tm_mday is
  interpreted as meaning the last day of the preceding month.
  这里的意思是，在将分解结构tm转换成time_t时（例如mktime），如果将
  tm_mday设为0则表示当前天数是前一个月的最后一天 */
  date.yday = (l_ushort)(st.tm_yday + 1); /* tm_yday - from 0 to 365 */
  date.day = (l_byte)st.tm_mday; /* tm_mday - from 1 to 31 */
  date.hour = (l_byte)st.tm_hour; /* tm_hour - from 0 to 23 */
  date.min = (l_byte)st.tm_min; /* tm_min - from 0 to 59 */
  date.sec = (l_byte)st.tm_sec; /* tm_sec - from 0 to 60, 60 can be leap second */
  /* Single UNIX Specification 的以前版本允许双闰秒，于是tm_sec值的有效范围是
  0到61。但是UTC的正式定义不允许双闰秒，所以现在tm_sec值的有效范围是0到60。*/
  if (st.tm_isdst > 0) {
    /* daylight saving time is in effect. gmtime_r将时间转换成协调统一时间的分解
    结构，不会像localtime_r那样考虑本地时区和夏令时，该值应该在此处无效 */
    l_logw_s("gmtime_r invalid tm_isdst");
  }
  return date;
}

l_date l_date_from_secs(l_long utcsecs) {
  return llgetdate(utcsecs);
}

l_date l_date_from_time(l_time utc) {
  l_date date = llgetdate(utc.sec);
  date.nsec = utc.nsec;
  return date;
}

l_date l_system_date() {
  return l_date_from_time(l_system_time());
}

/** File and attribute */

int l_is_file_exist(const void* name) {
  /**
   * # access, faccessat - check user's permissions for a file
   *
   * ```
   * #include <fcntl.h>
   * #include <unistd.h>
   * int access(const char* pathname, int mode);
   * int faccessat(int dirfd, const char* pathname, int mode, int flags);
   * ```
   *
   * **access**() checks whether the calling process can access the file
   * `pathname`. If `pathname` is a symbolic link, it is dereferenced.
   *
   * The `mode` specifies the accessibility check(s) to be performed, and is
   * either the value F_OK, or a mask consisting of the bitwise OR of one
   * or more of R_OK, W_OK, and X_OK. F_OK tests for the existence of the
   * file. R_OK, W_OK, and X_OK test whether the file exists and grants
   * read, write, and execute permissions, respectively.
   *
   * The check is done using the calling process's **real** UID and GID,
   * rather than the effective IDs as is done when actually attempting an
   * operation (e.g., open(2)) on the file. Similarly, for the root user,
   * the check uses the set of permitted capabilities rather than the set
   * of effective capabilities; and for non-root users, the check uses an
   * empty set of capabilities.
   *
   * This allows set-user-ID programs and capability-endowed programs to
   * easily determine the invoking user's authority. In other words,
   * **access**() does not answer the "can I read/write/execute this file?"
   * question. It answers a slightly different question: "(assuming I'm a
   * setuid binary) can the user who invoked me read/write/execute this
   * file?", which gives set-user-ID programs the possibility to prevent
   * malicious users from causing them to read files which users shouldn't
   * be able to read.
   *
   * If the calling process is privileged (i.e., its real UID is zero),
   * then an X_OK check is successful for a regular file if execute
   * permission is enabled for any of the file owner, group, or other.
   *
   * The **faccessat**() system call operates in exactly the same way as
   * **access**(), except for the differences described here. If the
   * pathname given is relative, then it is interpreted relative to the
   * directory referred to by the file descriptor `dirfd` (rather then
   * relative to the current working directory of the calling process,
   * as is done by access() for a relative pathname).
   *
   * If `pathname` is relative and `dirfd` is the special value AT_FDCWD, then
   * `pathname` is interpreted relative to the current working directory of
   * the calling process like access().
   *
   * If `pathname` is absolute, then `dirfd` is ignored. `flags` is constructed
   * by OR together zero or more of the following values:
   * AT_EACCESS - Perform access checks using the effective user and group IDs.
   * By default, faccessat() uses the real IDs like access().
   * AT_SYMLINK_NOFOLLOW - If `pathname` is a symbolic link, do not dereference it:
   * instead return information about the link itself.
   *
   * @return On success, zero is returned. On error, -1 is returned, and errno
   * is set appropriately.
   *
   */
   if (faccessat(AT_FDCWD, (const char*)name, F_OK, AT_SYMLINK_NOFOLLOW) != 0) {
     l_loge_1("faccessat %s", lserror(errno));
     return false;
   }
   return true;
}

int l_is_file_exist_in(l_filedescriptor dirfd, const void* name) {
  if (faccessat(dirfd.unifd, (const void*)name, F_OK, AT_SYMLINK_NOFOLLOW) != 0) {
    l_loge_1("faccessat %s", lserror(errno));
    return false;
  }
  return true;
}

l_filedescriptor l_get_dirfd(const void* name) {
  /**
   * # open, openat, creat - open and possibly create a file
   *
   * ```
   * #include <sys/types.h>
   * #include <sys/stat.h>
   * #include <fcntl.h>
   * int open(const char* pathname, int flags);
   * int open(const char* pathname, int flags, mode_t mode);
   * int creat(const char* pathname, mode_t mode);
   * int openat(int dirfd, const char* pathname, int flags);
   * int openat(int dirfd, const char* pathname, int flags, mode_t mode);
   * ```
   *
   * @return open(), openat(), and creat() return the new file descriptor, or -1
   * if an error occurred (in which case, errno is set appropriately).
   *
   */
   l_filedescriptor fd;
   fd.unifd = -1;

   if (!name) return fd;

   if ((fd.unifd = open((const char*)name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOATIME)) == -1) {
     l_loge_1("open %d", lserror(errno));
   }

   return fd;
}

void l_close_fd(l_filedescriptor fd) {
  if (fd.unifd == -1) return;
  if (close(fd.unifd) != 0) {
    l_loge_1("close %d", lserror(errno));
  }
  fd.unifd = -1;
}

int l_is_fd_valid(l_filedescriptor fd) {
  return (fd.unifd != -1);
}

l_long l_file_size(const void* name) {
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
  if (!name) { l_loge_s("empty name"); return 0; }
  l_zero_n(&st, sizeof(struct stat));
  if (lstat((const char*)name, &st) != 0) {
    l_loge_2("lstat %s %s", lserror(errno), ls(name));
    return 0;
  }
  return (l_long)st.st_size;
}

l_fileattr l_file_attr(const void* name) {
  struct stat st;
  l_fileattr fa = {0};
  if (!name) { l_loge_s("empty name"); return fa; }
  l_zero_n(&st, sizeof(struct stat));
  if (lstat((const char*)name, &st) != 0) {
    l_loge_2("lstat %s %s", lserror(errno), ls(name));
    return fa;
  } /* the time stored in stat is measured in seconds since 00:00:00 UTC, January 1, 1970 */
  fa.fsize = (l_long)st.st_size;
  fa.ctm = (l_long)st.st_ctime;
  fa.atm = (l_long)st.st_atime;
  fa.mtm = (l_long)st.st_mtime;
  fa.gid = (l_long)st.st_gid;
  fa.uid = (l_long)st.st_uid;
  fa.mode = (l_long)st.st_mode;
  fa.isfile = (l_byte)(S_ISREG(st.st_mode) != 0);
  fa.isdir = (l_byte)(S_ISDIR(st.st_mode) != 0);
  fa.islink = (l_byte)(S_ISLNK(st.st_mode) != 0);
  return fa;
}

l_dirstream l_open_dir(const void* name) {
  /** opendir **
  #include <sys/types.h>
  #include <dirent.h>
  DIR* opendir(const char* name);
  It opens a directory stream corresponding to the name, and returns
  a pointer to the directory stream. The stream is positioned at the
  first entry in the directory. Filename entries can be read from a
  directory stream using readdir(3). */
  l_dirstream d = {0};
  if (!name) { l_loge_s("empty name"); return d; }
  if ((d.stream = opendir((const char*)name)) == 0) {
    l_loge_1("opendir %s", lserror(errno));
  }
  return d;
}

void l_close_dir(l_dirstream* d) {
  if (d->stream) {
    if (closedir((DIR*)d->stream) != 0) {
      l_loge_1("closedir %s", lserror(errno));
    }
    d->stream = 0;
  }
}

const l_rune* l_read_dir(l_dirstream* d) {
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
  struct dirent* entry = 0;
  errno = 0;
  if ((entry = readdir((DIR*)d->stream)) == 0) {
    if (errno != 0) { l_loge_1("readdir %s", lserror(errno)); }
    return 0;
  }
  return l_rstr(entry->d_name);
}

int l_print_current_dir(void* stream, int (*write)(void* stream, const void* str)) {
  /** getcwd, get_current_dir_name - get current working directory
  #include <unistd.h>
  char *get_current_dir_name(void);
  it will malloc(3) an array big enough to hold the absolute pathname of the current
  working directory. if the environment variable PWD is set, and its value is correct,
  then that value will be returned. the caller should free(3) the returned buffer. */
  char* curdir = get_current_dir_name();
  int count = 0;
  if (curdir == 0) {
    l_loge_1("get_current_dir_name %s", lserror(errno));
    return 0;
  }
  count = write(stream, curdir);
  free(curdir);
  return count;
}

int l_execute_shell_command(const void* command, void (*out)(void*, const l_byte*, l_int), void* userobj) {
  /**
   * popen, pclose - pipe stream to or from a process
   * ```
   * FILE* popen(const char* command, const char* type);
   * int pclose(FILE* stream);
   * ```
   *
   * The popen() function opens a process by creating a pipe, forking, and
   * invoking the shell. Since a pipe is by definition unidirectional,
   * the type argument may specify only reading or writing, not both; the
   * resulting stream is correspondingly read-only or write-only.
   *
   * The command argument is a pointer to a null-terminated string
   * containing a shell command line. This command is passed to /bin/sh
   * using the -c flag; interpretation, if any, is performed by the shell.
   *
   * ```
   * $ sh -c command_string [command_name [argument ...]]
   * ```
   *
   * `man sh` -c: Read commands from the command_string operand instead of
   * from the standard input. Special parameter 0 will be set from the
   * command_name operand and the positional parameters ($1, $2, etc.) set
   * from the remaining argument operands.
   *
   * The type argument is a pointer to a null-terminated string which must
   * contain either the letter 'r' for reading or the letter 'w' for writing.
   * Since glibc 2.9, this argument can additionally include the letter 'e',
   * which causes the close-on-exec flag (FD_CLOEXEC) to be set on the
   * underlying file descriptor; see the description of the O_CLOEXEC flag
   * in open(2) for reasons why this may be useful.
   *
   * The return value from popen() is a normal standard I/O stream in all
   * respects save that it must be closed with pclose() rather than fclose(3).
   * Writing to such a stream writes to the standard input of the command;
   * the command's standard output is the same as that of the process that
   * called popen(), unless this is altered by the command itself. Conversely,
   * reading from the stream reads the command's standard output, and the
   * command's standard input is the same as that of the process that called
   * popen().
   *
   * Note that output popen() streams are block buffered by default. The
   * pclose() function waits for the associated process to terminate and
   * returns the exit status of the command as returned by wait4(2).
   *
   * @return popen() return NULL if failed; pclose() resturns the exit
   * status of the command on success, and return -1 if error.
   * Both functions set errno to an appropriate value in the case of an error.
   * If the type argument is invalid, and this condition is detected, errno
   * is set to EINVAL. If pclose() cannot obtain the child status, errno
   * is set to ECHILD. The popen() function doesn't set errno if memory
   * allocation fails.
   *
   * Since the standard input of a command opened for reading shares its seek
   * offset with the process that called popen(), if the original process
   * has done a buffered read, the command's input position may not be as
   * expected. Similarly, the output from a command opened for writing may
   * become intermingled with that of the original process. The later can be
   * avoided by calling fflush(3) before popen().
   *
   * Failure to execute the shell is indistinguishable from the shell's failure
   * to execute command, or an immediate exit of the command. The only hint is
   * an exit status of 127.
   */

   int exitcode = 0;
   l_filestream fs;
   #define SHELL_OUTPUT_TEMP_BUFSZ 1024
   l_byte buffer[SHELL_OUTPUT_TEMP_BUFSZ+1];
   l_int readsize = 0;

   fs.stream = popen((const char*)command, "r");
   if (!fs.stream) {
     l_loge_1("popen failed %s", lserror(errno));
     return false;
   }

   while ((readsize = l_read_file(&fs, buffer, SHELL_OUTPUT_TEMP_BUFSZ)) == SHELL_OUTPUT_TEMP_BUFSZ) {
     out(userobj, buffer, readsize);
   }

   if (readsize > 0) {
     out(userobj, buffer, readsize);
   }

   exitcode = pclose(fs.stream);
   if (exitcode == -1) {
     l_loge_1("pclose failed %s", lserror(errno));
     return false;
   }

   if (exitcode != 0) {
     l_loge_1("pclose exit code %d", ld(exitcode));
     return false;
   }

   return true;
}

void l_thrkey_init(l_thrkey* self) {
  /** pthread_key_create - thread-specific data key creation **
  #include <pthread.h>
  int pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
  If successful, it shall store the newly created key value at *key and
  shall return zero. Otherwise, an error number shall be returned to
  indicate the error. */
  int n = pthread_key_create((pthread_key_t*)self, 0);
  if (n != 0) {
    l_loge_1("pthread_key_create %s", lserror(n));
  }
}

void l_thrkey_free(l_thrkey* self) {
  /** pthread_key_delete - thread-specific data key deletion **
  #include <pthread.h>
  int pthread_key_delete(pthread_key_t key); */
  int n = pthread_key_delete(*(pthread_key_t*)self);
  if (n != 0) {
    l_loge_1("pthread_key_delete %s", lserror(n));
  }
}

void* l_thrkey_get_data(l_thrkey* self) {
  /** thread-specific data management **
  #include <pthread.h>
  void* pthread_getspecific(pthread_key_t key);
  int pthread_setspecific(pthread_key_t key, const void* value);
  No errors are returned from pthread_getspecific(), pthread_setspecific() may fail:
  ENOMEM - insufficient memory exists to associate the value with the key.
  EINVAL - the key value is invalid. */
  return pthread_getspecific(*(pthread_key_t*)self);
}

void l_thrkey_set_data(l_thrkey* self, const void* data) {
  /* different threads may bind different values to the same key, the value
  is typically a pointer to blocks of dynamically allocated memory that have
  been reserved for use by the calling thread. */
  int n = pthread_setspecific(*(pthread_key_t*)self, data);
  if (n != 0) {
    l_loge_1("pthread_setspecific %s", lserror(n));
  }
}

void l_mutex_init(l_mutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_init(mutex, 0);
  if (n != 0) {
    l_loge_1("pthread_mutex_init %s", lserror(n));
  }
}

void l_mutex_free(l_mutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_destroy(mutex);
  if (n != 0) {
    l_loge_1("pthread_mutex_destroy %s", lserror(n));
  }
}

void l_mutex_lock(l_mutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_lock(mutex);
  if (n != 0) {
    l_loge_1("pthread_mutex_lock %s", lserror(n));
  }
}

void l_mutex_unlock(l_mutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_unlock(mutex);
  if (n != 0) {
    l_loge_1("pthread_mutex_unlock %s", lserror(n));
  }
}

int l_mutex_trylock(l_mutex* self) {
  pthread_mutex_t* mutex = (pthread_mutex_t*)self;
  int n = pthread_mutex_trylock(mutex);
  if (n == 0) {
    return true;
  }
  if (n != EBUSY) {
    l_loge_1("pthread_mutex_trylock %s", lserror(n));
  }
  return false;
}

void l_rwlock_init(l_rwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_init(lock, 0);
  if (n != 0) {
    l_loge_1("pthread_rwlock_init %s", lserror(n));
  }
}

void l_rwlock_free(l_rwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_destroy(lock);
  if (n != 0) {
    l_loge_1("pthread_rwlock_destroy %s", lserror(n));
  }
}

void l_rwlock_read(l_rwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_rdlock(lock);
  if (n != 0) {
    l_loge_1("pthread_rwlock_rdlock %s", lserror(n));
  }
}

void l_rwlock_write(l_rwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_wrlock(lock);
  if (n != 0) {
    l_loge_1("pthread_rwlock_wrlock %s", lserror(n));
  }
}

int l_rwlock_tryread(l_rwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_tryrdlock(lock);
  if (n != 0) {
    l_loge_1("pthread_rwlock_tryrdlock %s", lserror(n));
    return false;
  }
  return true;
}

int l_rwlock_trywrite(l_rwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_trywrlock(lock);
  if (n != 0) {
    l_loge_1("pthread_rwlock_trywrlock %s", lserror(n));
    return false;
  }
  return true;
}

void l_rwlock_unlock(l_rwlock* self) {
  pthread_rwlock_t* lock = (pthread_rwlock_t*)self;
  int n = pthread_rwlock_unlock(lock);
  if (n != 0) {
    l_loge_1("pthread_rwlock_unlock %s", lserror(n));
  }
}

void l_condv_init(l_condv* self) {
  pthread_cond_t* cond = (pthread_cond_t*)self;
  int n = pthread_cond_init(cond, 0);
  if (n != 0) {
    l_loge_1("pthread_cond_init %s", lserror(n));
  }
}

void l_condv_free(l_condv* self) {
  pthread_cond_t* cond = (pthread_cond_t*)self;
  int n = pthread_cond_destroy(cond);
  if (n != 0) {
    l_loge_1("pthread_cond_destroy %s", lserror(n));
  }
}

void l_condv_wait(l_condv* self, l_mutex* mutex) {
  pthread_cond_t* c = (pthread_cond_t*)self;
  pthread_mutex_t* m = (pthread_mutex_t*)mutex;
  int n = pthread_cond_wait(c, m);
  if (n != 0) {
    l_loge_1("pthread_cond_wait %s", lserror(n));
  }
}

int l_condv_timedwait(l_condv* self, l_mutex* mutex, l_long ns) {
  pthread_cond_t* c = (pthread_cond_t*)self;
  pthread_mutex_t* m = (pthread_mutex_t*)mutex;
  l_time curtime = l_system_time();
  struct timespec tm;
  int n = 0;

  /* caculate the absolute time */
  ns += curtime.nsec + curtime.sec * l_nsecs_per_second;
  if (ns < 0) {
    ns = 0;
    l_loge_s("invalid timeout value");
  }
  tm.tv_sec = (time_t)(ns / l_nsecs_per_second);
  tm.tv_nsec = (long)(ns - tm.tv_sec * l_nsecs_per_second);

  /* int pthread_cond_timedwait(pthread_cond_t* cond,
  pthread_mutex_t* mutex, const struct timespec* abstime); */
  n = pthread_cond_timedwait(c, m, &tm);
  if (n == 0 || n == ETIMEDOUT) {
    return true;
  }
  l_loge_1("pthread_cond_timedwait %d", lserror(n));
  return false;
}

void l_condv_signal(l_condv* self) {
  pthread_cond_t* cond = (pthread_cond_t*)self;
  /* pthread_cond_signal() shall unblock at least one of the threads
  that are blocked on the specified condition variable cond (if any
  threads are blocked on cond).
  pthread_cond_broadcast() and pthread_cond_signal() functions shall
  have no effect if there are no threads currently blocked on cond.
  It is not safe to use the pthread_cond_signal() function in a signal
  handler that is invoked asynchronously. Even if it were safe, there
  would still be a race between the test of the Boolean pthread_cond_wait()
  that could not be efficiently eliminated.
  Mutexes and condition variables are thus not suitable for releasing
  a waiting thread by signaling from code running in a signal handler. */
  int n = pthread_cond_signal(cond);
  if (n != 0) {
    l_loge_1("pthread_cond_signal %s", lserror(n));
  }
}

void l_condv_broadcast(l_condv* self) {
  pthread_cond_t* cond = (pthread_cond_t*)self;
  int n = pthread_cond_broadcast(cond);
  if (n != 0) {
    l_loge_1("pthread_cond_broadcast %s", lserror(n));
  }
}

l_thrid l_raw_thread_self() {
  l_thrid thrid;
  *((pthread_t*)&thrid) = pthread_self();
  return thrid;
}

int l_raw_thread_create(l_thrid* thrid, void* (*start)(void*), void* para) {
  pthread_t* thread = (pthread_t*)thrid;
  int n = pthread_create(thread, 0, start, para);
  if (n != 0) {
    l_loge_1("pthread_create %s", lserror(n));
    return false;
  }
  return true;
}

void l_raw_thread_cancel(l_thrid* thrid) {
  /* pthread_cancel - send a cancellation request to a thread
  #include <pthread.h>
  int pthread_cancel(pthread_t thread);
  On success, it returns 0; on error, it returns a nonzero
  error number */
  int n = pthread_cancel(*((pthread_t*)thrid));
  if (n != 0) {
    l_loge_1("pthread_cancel %s", lserror(errno));
  }
}

void l_raw_thread_sleep(l_long us) {
  struct timespec req;
  req.tv_sec = (time_t)(us/1000000);
  req.tv_nsec = (long)(us%1000000*1000);
  if (nanosleep(&req, 0) != 0) {
    if (errno != EINTR) {
      l_loge_1("nanosleep %s", lserror(errno));
    }
  }
}

void l_raw_thread_exit() {
  pthread_exit((void*)1);
}

int l_raw_thread_join(l_thrid* thrid) {
  int n = 0;
  void* exitcode = 0;
  pthread_t* thread = (pthread_t*)thrid;
  /* wait thread terminate, if already terminated then return
  immediately. the thread needs to be joinable. join a alreay
  joined thread will results in undefined behavior. */
  if ((n = pthread_join(*thread, &exitcode)) != 0) {
    l_loge_1("pthread_join %s", lserror(n));
  }
  return (int)(l_int)exitcode;
}

#if defined(L_PLAT_LINUX)
#include "linuxionf.c"
#elif defined(L_PLAT_APPLE) || defined(L_PLAT_BSD)
#include "bsdkqueue.c"
#else
#include "linuxpoll.c"
#endif

void l_plat_test() {
  l_assert(sizeof(l_mutex) >= sizeof(pthread_mutex_t));
  l_assert(sizeof(l_rwlock) >= sizeof(pthread_rwlock_t));
  l_assert(sizeof(l_condv) >= sizeof(pthread_cond_t));
  l_assert(sizeof(int) == sizeof(l_umedit)); /* test file descriptor size */
  l_logd_1("pthread_t %d-byte", ld(sizeof(pthread_t)));
  l_logd_1("pthread_mutext_t %d-byte", ld(sizeof(pthread_mutex_t)));
  l_logd_1("pthread_rwlock_t %d-byte", ld(sizeof(pthread_rwlock_t)));
}

