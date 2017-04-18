#include "thatcore.h"
int mainentry() {
  core_thattest();
  core_plattest();
  return 0;
}

/** 参考资料 ***
[1] UNIX环境高级编程 第6.10节 时间和日期例程 
[2] LINUX/UNIX系统编程手册 第10章 时间
[3] LINUX/UNIX系统编程手册 第23章 定时器与休眠
[4] http://man7.org/linux/man-pages/man7/time.7.html
[5] http://man7.org/linux/man-pages/man2/clock_gettime.2.html
[6] http://man7.org/linux/man-pages/man2/gettimeofday.2.html
[7] http://man7.org/linux/man-pages/man2/time.2.html
[8] http://man7.org/linux/man-pages/man3/ctime.3.html **/
struct cctime llgettime(clockid_t id) {
  struct timespec spec = {0};
  struct cctime time = {0};
  if (clock_gettime(id, &spec) != 0) {
    ccloge("clock_gettime %d %s", id, strerror(errno));
    return time;
  }
  time.secs = (_int)spec.tv_sec;
  time.ns = (medit)spec.tv_nsec;
  return time;
}
struct cctime ccgetsystime() {
  /** clock_gettime ***
  #include <sys/time.h>
  int clock_gettime(clockid_t id, struct timespec* out);
  @id为CLOCK_REALTIME时，clock_gettime提供了与time函数相同的功能，不
  过当系统支持高精度时间时，clock_gettime返回精度更高的实时系统时间；
  System-wide clock that measures real (i.e., wall-clock) time.
  Setting this clock requires appropriate privilege. This clock is
  affected by discontinuous jumps in the system time (e.g., if the
  system adaministrator manually changes the clock), and by the
  incremental adjustments performed by adjtime and NTP.
  *** gettimeofday ***
  #include <sys/time.h>
  int gettimeofday(struct timeval* tp, void* tz);
  @@SUSv4指定该函数已被弃用，但一些程序仍然使用这个函数，因为与time
  相比，gettimeofday提供了跟高的时间精度（微妙级）；
  @tz的唯一合法值是NULL，其他值将产生不确定的结果，某些平台支持用tz
  说明时区，但这完全依赖实现，SUS对此并没有定义
  @tp会保存获取到的系统时间，tp->tv_secs是time_t类型值，是相对于1970
  年1月1日00:00:00+0000以来的秒数，调用gmtime可以将time_t类型值转换成
  UTC标准日历时间，而调用localtime可转换成本地日历时间 **/
  return llgettime(CLOCK_REALTIME);
}
static struct ccdate llgetsysdate(time_t utcsecs, int tz) {

}
struct ccdate ccgetsysdate(int tz) {
  struct cctime utc = ccgetsystime();
  return llgetsysdate(utc.secs, tz);
}
struct ccdate ccgetsysdatefrom(struct cctime* utc, int tz) {
  return llgetsysdate(utc->secs, tz);
}
struct cctime ccgetboottime() {
  /** CLOCK_MONOTONIC/CLOCK_BOOTTIME ***
  CLOCK_MONOTONIC cannot be set and represents monotonic time
  since some unspecified starting point. This clock is not
  affected by discontinuous jumps in the system time, but is
  affected by the incremental adjustments performed by adjtime
  and NTP.
  CLOCK_BOOTTIME (since Linux 2.6.39, Linux-specific) is
  identical to CLOCK_MONOTONIC, except it also includes any time
  that the system is suspended. This allows applications to get
  a suspend-aware monotonic clock without having to deal with
  the complications of CLOCK_REALTIME, which may have discontinuities
  if the time is changed using settimeofday or similar. **/
  clockid_t id = CLOCK_MONOTONIC;
#ifdef CC_OS_LINUX
  id = CLOCK_BOOTTIME;
#endif
  return llgettime(id);
}
struct cctime ccgetthredtime() {
  return llgettime(CLOCK_THREAD_CPUTIME_ID);
}
struct cctime ccgetprocesstime() {
  return llgettime(CLOCK_PROCESS_CPUTIME_ID);
}
static _int llgetres(clockid_t id) {
  struct timespec spec = {0};
  if (clock_getres(id, &spec) != 0) {
    ccloge("clock_getres %s", strerror(errno));
    return 0;
  }
  return (_int)spec.tv_sec*1000000000LL + (_int)spec.tv_nsec;
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
