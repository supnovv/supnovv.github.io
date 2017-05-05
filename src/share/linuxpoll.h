#ifndef CCLIB_LINUXPOLL_H_
#define CCLIB_LINUXPOLL_H_

#if defined(CC_OS_LINUX)
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <sys/epoll.h>
/* epoll */

struct llepollmgr {
  int epfd;
  int n, maxlen;
  struct epoll_event ready[CCPOLL_WIAT_MAX_EVENTS];
};


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

