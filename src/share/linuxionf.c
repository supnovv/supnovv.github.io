#include <string.h>
#include <errno.h>
#include "plationf.h"
#include "ionotify.h"

#if defined(CC_OS_LINUX)
/** Linux Epoll **/

/** eventfd - create a file descriptor for event notification **
#include <sys/eventfd.h>
int eventfd(unsigned int initval, int flags);
Creates an "eventfd object" that can be used as an event wait/notify
mechanism by user-space applications, and by the kernel to notify
user-space applications of events. The object contains an unsigned
64-bit integer (uint64_t) counter that is maintained by the kernel.
This counter is initialized with the value specified in the
argument initval.
eventfd() is available on Linux since kernel 2.6.22. Working support
is provided in glibc since version 2.8. The eventfd2() system call
is available on Linux since kernel 2.6.27. Since version 2.9, the
glibc eventfd() wrapper will employ the eventfd2() system call,
if it is supported by the kernel. eventfd() and eventfd2() are
Linux-specific.
The following values may be bitwise ORed in flags to change the
behavior of eventfd():
EFD_CLOEXEC (since Linux 2.6.27) - set the close-on-exec (FD_CLOEXEC)
flag on the new file descriptor.
EFD_NONBLOCK (since Linux 2.6.27) - set the O_NONBLOCK file status
flag on the new open fd. Using this flag saves extra calls to fcntl(2)
to achieve the same result.
EFD_SEMAPHORE (since Linux 2.6.30) - provided semaphore-like semantics
for reads from the new fd.
In Linux up to version 2.6.26, the flags arugment is unused, and must
be specified as zero.
---
It returns a new fd that can be used to refer to the eventfd object. The
following operations can be perfromed on the file descriptor.
* read(2) - each successful read(2) returns an 8-byte integer. A read(2)
will fail with the error EINVAL if the size of the supplied buffer is less
than 8 bytes. The value returned by read(2) is host type order, i.e., the
native byte order for integers on the host machine.
The semantics of read(2) depend on whether the evenfd counter currently
has a nonzero value and whether the EFD_SEMAPHORE flag was specified when
creating the evenfd file descriptor:
If EFD_SEMAPHORE was not specified and the eventfd counter has a nonzero
value, than a read(2) returns 8 bytes containing that value, and the
counter's value is reset to zero.
If EFD_SEMAPHORE was specified and eventfd counter has a nonzero value,
then a read(2) returns 8 bytes containing the value 1, and the counter's
value is decremented by 1.
If the eventfd counter is zero at the time of the call to read(2), then
the call either blocks until the counter becomes nonzero (at which time,
the read(2) proceeds as described above) or fails with the error EAGAIN
if the file descriptor has been made nonblocking.
* write(2) - a write(2) call adds the 8-byte integer value supplied in
its buffer to the counter. The maximum value that may be stored in the
counter is the largest unsigned 64-bit value minux 1 (i.e., 0xfffffffe).
If the addition would cause the counter's value to exceed the maximum,
then the write(2) either blocks until a read(2) is performed on the fd.
or fails with the error EAGAIN if the fd has been made nonblocking.
A write(2) will fail with the error EINVAL if the size of the supplied
buffer is less than 8 bytes, or if an attempt is made to write the value
0xffffffff.
* poll(2), select(2) and similar - the returned fd supports poll(2),
epoll(7) and select(2), as follows:
The fd is readable if the counter has a value greater than 0. The fd is
writable if it is possible to write a value of at least "1" without blocking.
If an overflow of the counter value was detected, then select(2) indicates
the fd as being both readable and writable, and poll(2) returns a POLLERR
event. As noted above, write(2) can never overflow the counter. However an
overflow can occur if 2^64 evenfd "signal posts" were performed by the KAIO
subsystem (theoretically possible, but practically unlikely). If an overflow
has occured, then read(2) will return that maximum uint64_t value (i.e,
0xffffffff). The eventfd fd also supports the other fd multiplexing APIs:
pselect(2) and ppoll(2).
* close(2) - when the fd is no longer required it shoud be closed. When all
fd associated with the same eventfd object have been closed, the resources
for object are freed by the kernel.
---
On success, eventfd() returns a new eventfd. On error, -1 is returned and
errno is set to indicate the error.
EINVAL - an unsupported value was specified in flags.
EMFILE - the per-process limit on open fd has been reached.
ENFILE - the system-wide limit on the total number of open files has been reached.
ENODEV - could not mount (internal) anonymous inode device.
ENOMEM - there was insufficent memory to create a new eventfd file descriptor.
Application can use an eventfd file descriptor instead of a pipe in all cases where
a pipe is used simply to signal events. The kernel overhead of an eventfd is
much lower than that of a pipe, and only one fd is required (versus the two required
for pipe). When used in the kernel, an eventfd descriptor can provide a bridge
from kernel to user space, allowing, for example, functionalities like (KAIO, kernel
AIO) to signal to a file descriptor that some operation is complete.
A key point about an eventfd is that it can be monitored just like any other fd
using select(2), poll(2), or epoll(7). This means that an application can simultaneously
monitor the readiness of "traditonal" files and the readiness of other kernel
mechanisms that support the eventfd interface. (Without the eventfd() interface, these
mechanisms could not be multiplexed via select(2), poll(2), or epoll(7)). */

/* return -1 failed, others success */
static int ll_event_fd() {
  int n = eventfd(0, EFD_NONBLOCK);
  if (n == -1) {
    ccloge("eventfd %s", strerror(n));
  }
  return n;
}

static void ll_event_fd_close(handle_int fd) {
  if (close(fd) != 0) {
    ccloge("eventfd close %s", strerror(errno));
  }
}

sright_int ll_read(handle_int fd, void* out, sright_int count);
sright_int ll_write(handle_int fd, const void* buf, sright_int count);

/* return > 0 success, -1 block, -2 error */
static int ll_event_fd_write(int fd) {
  uright_int count = 1;
  int n = ll_write(fd, &count, sizeof(uright_int));
  if (n == sizeof(uright_int)) {
    return n;
  }
  if (n == -1) return n;
  return -2;
}

/* return > 0 success, -1 block, -2 error */
static int ll_event_fd_read(int fd) {
  uright_int count = 0;
  int n = ll_read(fd, &count, sizeof(uright_int));
  if (n == sizeof(uright_int)) {
    return (int)count;
  }
  if (n == -1) return n;
  return -2;
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
epoll fd in the process's /proc/[pid]/fdinfo directory. */

static int ll_epoll_create() {
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
  int epfd = epoll_create1(0);
  if (epfd == -1) {
    ccloge("epoll_create1 %s", strerror(errno));
  }
  return epfd;
}

static void llepollmgr_close(int epfd) {
  if (close(epfd) != 0) {
    ccloge("close epoll %s", strerror(errno));
  }
}

static nauty_bool llepollmgr_ctl(int epfd, int op, int fd, struct epoll_event* event) {
  if (epfd == -1 || fd == -1 || epfd == fd) {
    ccloge("llepollmgr_ctl invalid fd");
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
  event->events |= EPOLLET; /* edge-triggered */
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
  return (epoll_ctl(epfd, op, fd, event) != 0);
}

static nauty_bool llepollmgr_add(int epfd, int fd, struct epoll_event* event) {
  if (!llepollmgr_ctl(epfd, EPOLL_CTL_ADD, fd, event)) {
    if (errno != EEXIST || !llepollmgr_ctl(epfd, EPOLL_CTL_MOD, fd, event)) {
      ccloge("llepollmgr_add %s", strerror(errno));
      return false;
    }
  }
  return true;
}

static nauty_bool llepollmgr_mod(int epfd, int fd, struct epoll_event* event) {
  if (!llepollmgr_ctl(epfd, EPOLL_CTL_MOD, fd, event)) {
    ccloge("llepollmgr_mod %s", strerror(errno));
    return false;
  }
  return true;
}

static nauty_bool llepollmgr_del(int epfd, int fd) {
  /* In kernel versions before 2.6.9, the EPOLL_CTL_DEL
  operation required a non-null pointer in event, even
  though this argument is ignored. Since Linux 2.6.9, event
  can be specified as NULL when using EPOLL_CTL_DEL.
  Applications that need to be portable to kernels before
  2.6.9 should specify a non-null pointer in event. */
  struct epoll_event event;
  cczeron(&event, sizeof(struct epoll_event));
  if (!llepollmgr_ctl(epfd, EPOLL_CTL_DEL, fd, &event)) {
    ccloge("llepollmgr_del %s", strerror(errno));
    return false;
  }
  return true;
}

static void llepollmgr_wait(struct llepollmgr* self, int ms) {
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
  int n = 0;

  errno = 0;
  if ((self->nready = epoll_wait(self->epfd, self->ready, CCEPOLL_MAX_EVENTS, ms)) < 0) {
    self->nready = 0;
  }

  n = errno;
  if (n == EINTR) {
    cclogw("epoll_wait interrupted %s", strerror(n));
  } else if (n != 0) {
    ccloge("epoll_wait %s", strerror(errno));
  }
}

nauty_bool ccionfmgr_init(struct ccionfmgr* self) {
  struct llepollmgr* mgr = (struct llepollmgr*)self;
  cczeron(mgr, sizeof(struct llepollmgr));
  ccmutex_init(&(mgr->mutex));
  if ((mgr->epfd = ll_epoll_create()) == -1) {
    return false;
  }
  if ((mgr->wakeupfd = ll_event_fd()) == -1) {
    return false;
  }
  return true;
}

void ccionfmgr_free(struct ccionfmgr* self) {
  struct llepollmgr* mgr = (struct llepollmgr*)self;
  mgr->wakeupfd_added = false;
  mgr->wakeup_count = 0;
  ccmutex_free(&(mgr->mutex));
  if (mgr->epfd != -1) {
    llepollmgr_close(mgr->epfd);
    mgr->epfd = -1;
  }
  if (mgr->wakeupfd != -1) {
    ll_event_fd_close(mgr->wakeupfd);
    mgr->wakeupfd = -1;
  }
}

static uint32_t llepollmasks[] = {
  /* 0x00 */ 0,
  /* 0x01 */ EPOLLIN,
  /* 0x02 */ EPOLLOUT,
  /* 0x03 */ 0,
  /* 0x04 */ EPOLLPRI,
  /* 0x05 */ 0,
  /* 0x06 */ 0,
  /* 0x07 */ 0,
  /* 0x08 */ EPOLLRDHUP,
  /* 0x09 */ 0,
  /* 0x0a */ 0,
  /* 0x0b */ 0,
  /* 0x0c */ 0,
  /* 0x0d */ 0,
  /* 0x0e */ 0,
  /* 0x0f */ 0,
  /* 0x10 */ EPOLLHUP,
  /* 0x11 */ 0
};

static umedit_int llionfrd[] = {0, CCIONFRD};
static umedit_int llionfwr[] = {0, CCIONFWR};
static umedit_int llionfpri[] = {0, CCIONFPRI};
static umedit_int llionfrdh[] = {0, CCIONFRDH};
static umedit_int llionfhup[] = {0, CCIONFHUP};
static umedit_int llionferr[] = {0, CCIONFERR};

static uint32_t llgetepollmasks(struct ccionfevt* event) {
  return (llepollmasks[event->masks & CCIONFRD] | llepollmasks[event->masks & CCIONFWR] |
    llepollmasks[event->masks & CCIONFPRI] | llepollmasks[event->masks & CCIONFRDH]);
}

static umedit_int llgetionfmasks(struct epoll_event* event) {
  uint32_t masks = event->events;
  return (llionfrd[(masks&EPOLLIN)!=0] | llionfwr[(masks&EPOLLOUT)!=0] | llionfpri[(masks&EPOLLPRI)!=0] |
    llionfrdh[(masks&EPOLLRDHUP)!=0] | llionfhup[(masks&EPOLLHUP)!=0] | llionferr[(masks&CCIONFERR)!=0]);
}

static uint64_t llgetepolludata(struct ccionfevt* event) {
  uint32_t hdl = event->hdl;
  uint64_t udata = event->udata;
  return ((udata << 32) | hdl);
}

static handle_int llgetionfhandle(struct epoll_event* event) {
  uint32_t hdl = (uint32_t)(event->data.u64 & 0xFFFFFFFF);
  return (handle_int)hdl;
}

static umedit_int llgetionfudata(struct epoll_event* event) {
  return (umedit_int)(event->data.u64 >> 32);
}

nauty_bool ccionfmgr_add(struct ccionfmgr* self, struct ccionfevt* event) {
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
  struct llepollmgr* mgr = (struct llepollmgr*)self;
  struct epoll_event e;
  e.events = (EPOLLHUP | EPOLLERR | llgetepollmasks(event));
  e.data.u64 = llgetepolludata(event);
  return llepollmgr_add(mgr->epfd, (int)event->hdl, &e);
}

nauty_bool ccionfmgr_mod(struct ccionfmgr* self, struct ccionfevt* event) {
  struct llepollmgr* mgr = (struct llepollmgr*)self;
  struct epoll_event e;
  e.events = (EPOLLHUP | EPOLLERR | llgetepollmasks(event));
  e.data.u64 = llgetepolludata(event);
  return llepollmgr_mod(mgr->epfd, (int)event->hdl, &e);
}

nauty_bool ccionfmgr_del(struct ccionfmgr* self, struct ccionfevt* event) {
  struct llepollmgr* mgr = (struct llepollmgr*)self;
  return llepollmgr_del(mgr->epfd, (int)event->hdl);
}

/* return > 0 success, -1 block, -2 error, -3 already signaled */
int ccionfmgr_wakeup(struct ccionfmgr* self) {
  struct llepollmgr* mgr = (struct llepollmgr*)self;
  nauty_bool needtowakeup = false;

  /* if we use a flag like "wait_is_called" to indicate master called epoll_wait() or not,
  and then write eventfd to signal master wakeup only "wait_is_called" is true, then master
  may not be triggered to wakeup. because if this kind of check is performed just before
  master call epoll_wait(), wakeup is not signaled but master will enter into wait state
  next. so dont use this trick. */

  /* here is the another trick to count the wakeup times.
  this function can be called from any thread, the counter
  need to be protected by a lock.*/
  ccmutex_lock(&(mgr->mutex));
  if (mgr->wakeup_count < 2) {
    mgr->wakeup_count += 1;
    needtowakeup = true;
  }
  ccmutex_unlock(&(mgr->mutex));

  if (!needtowakeup) {
    /* already signaled to wakeup */
    return -3;
  }

  return ll_event_fd_write(mgr->wakeupfd);
}

/* return number of events waited and handled */
int ccionfmgr_timedwait(struct ccionfmgr* self, int ms, void (*cb)(struct ccionfevt*)) {
  struct llepollmgr* mgr = (struct llepollmgr*)self;
  struct epoll_event* start = 0;
  struct epoll_event* beyond = 0;
  struct ccionfevt event;
  int nevent = 0, n = 0;

  /* timeout cannot be negative except infinity (-1) */
  if (ms < 0 && ms != -1) {
    ms = 0;
  }

  /* Linux may treat the timeout greater than
  35.79 minutes as infinity */
  if (ms > 30 * 60 * 1000) {
    ms = 30 * 60 * 1000; /* 30min */
  }

  if (!mgr->wakeupfd_added) {
    struct epoll_event e;
    mgr->wakeupfd_added = true;
    e.events = EPOLLERR | EPOLLIN;
    e.data.fd = mgr->wakeupfd;
    if (!llepollmgr_add(mgr->epfd, mgr->wakeupfd, &e)) {
      ccloge("epoll_wait add wakeupfd failed");
    }
  }

  llepollmgr_wait(mgr, ms);
  if (mgr->nready <= 0) {
    return 0;
  }

  start = mgr->ready;
  beyond = start + mgr->nready;
  for (; start < beyond; ++start) {
    if (start->data.fd == mgr->wakeupfd) {
      /* return > 0 success, -1 block, -2 error */
      n = ll_event_fd_read(mgr->wakeupfd);
      cclogd("ionf wakeup %s", ccitos(n));
      /* count down wakeup_count */
      ccmutex_lock(&(mgr->mutex));
      if (mgr->wakeup_count > 0) {
        mgr->wakeup_count -= 1;
      }
      ccmutex_unlock(&(mgr->mutex));
      continue;
    }
    nevent += 1;
    event.masks = llgetionfmasks(start);
    event.hdl = llgetionfhandle(start);
    event.udata = llgetionfudata(start);
    cb(&event);
  }

  return nevent;
}

int ccionfmgr_wait(struct ccionfmgr* self, void (*cb)(struct ccionfevt*)) {
  return ccionfmgr_timedwait(self, -1, cb);
}

int ccionfmgr_trywait(struct ccionfmgr* self, void (*cb)(struct ccionfevt*)) {
  return ccionfmgr_timedwait(self, 0, cb);
}

#elif defined(CC_OS_APPLE) || defined(CC_OS_BSD)
/** BSD Kqueue **/


#else
/** Linux Poll **/

struct ccepoll {
  struct ccarray fdset;
  struct cchashtbl fdud;
};

#define CCEPOLLINITFDBITS 6  /* 2 ^ 6 = 64 */
#define CCEPOLLINITFDSIZE (1 << CCEPOLLINITFDBITS)

void ccepollinit(struct ccepoll* self) {
  cczeron(self, sizeof(struct ccepoll));
}

nauty_bool ccepollcreate(struct ccepoll* self) {
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

nauty_bool ccepolladd(struct ccepoll* self, int fd, char flag) {
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

nauty_bool ccepollmod(struct ccepoll* self, int fd, char flag) {
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

#endif /* CC_OS_LINUX */

void ccplationftest() {
  ccassert(sizeof(handle_int) <= 4);
}

