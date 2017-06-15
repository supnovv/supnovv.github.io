#include "ionfmgr.h"

#if defined(L_PLAT_LINUX)
#include "linuxionf.c"
#elif defined(L_PLAT_APPLE) || defined(L_PLAT_BSD)
#include "bsdkqueue.c"
#elif !defined(L_PLAT_WINDOWS)
#include "linuxpoll.c"
#else
#include "winiocp.c"
#endif

