#ifndef lucy_plationf_h
#define lucy_plationf_h
#include "core/prefix.h"

#if defined(L_PLAT_LINUX)
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
#define L_IONFMGR_TYPE_SIZE sizeof(llepollmgr)
#define L_HANDLE_TYPE_SIZE sizeof(int)
#define L_HANDLE_TYPE_IS_SIGNED (1)
#endif

#elif defined(L_PLAT_APPLE) || defined(L_PLAT_BSD)
#include <sys/types.h>
#include <sys/event.h>
/** BSD Kqueue **/

#elif !defined(L_PLAT_WINDOWS)
#include "osi/linuxpref.h"
#include <poll.h>
/** Linux Poll **/

#else
/** Windows IO **/

#endif
#endif /* lucy_plationf_h */

