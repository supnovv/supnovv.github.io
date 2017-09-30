#ifndef lucy_plationf_h
#define lucy_plationf_h
#include "core/prefix.h"

#if defined(l_plat_linux)
#include "osi/linuxpref.h"
#include <sys/epoll.h>

#define L_EPOLL_MAX_EVENTS 64

typedef struct {
  int epfd;
  int wakeupfd;
  int nready;
  int wakeupfd_added;
  int wakeup_count;
  pthread_mutex_t mutex;
  struct epoll_event ready[L_EPOLL_MAX_EVENTS+1];
} llepollmgr;

#ifdef L_CORE_AUTO_CONFIG
#define L_EVENTMGR_TYPE_SIZE sizeof(llepollmgr)
#endif

#elif defined(l_plat_apple) || defined(l_plat_bsd)
#include <sys/types.h>
#include <sys/event.h>
/** BSD Kqueue **/

#elif !defined(l_plat_windows)
#include "osi/linuxpref.h"
#include <poll.h>
/** Linux Poll **/

#else
/** Windows IO **/

#endif
#endif /* lucy_plationf_h */

