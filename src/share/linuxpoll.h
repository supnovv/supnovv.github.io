#ifndef CCLIB_LINUXPOLL_H_
#define CCLIB_LINUXPOLL_H_

#if defined(CC_OS_LINUX)
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <sys/epoll.h>
/* epoll */

#define CCEPOLL_MAX_EVENTS 1024

struct llepollmgr {
  int epfd;
  int n, maxlen;
  struct epoll_event ready[CCEPOLL_MAX_EVENTS];
};

#ifdef CCLIB_AUTOCONF_TOOL
#define LLIONFMGR_TYPE_BYTES sizeof(struct llepollmgr)
#define LLIONFHDL_TYPE_BYTES sizeof(int)
#define LLIONFHDL_TYPE_IS_SIGNED (1)
#endif

#else
#if defined(CC_OS_APPLE) || defined(CC_OS_BSD)
#include <sys/types.h>
#include <sys/event.h>
/* kqueue */

#else
#include <poll.h>
/* poll */


#endif
#endif
#endif /* CCLIB_LINUXPOLL_H_ */

