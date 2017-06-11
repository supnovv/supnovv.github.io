#ifndef CCLIB_IONOTIFY_H_
#define CCLIB_IONOTIFY_H_
#include "thatcore.h"

#define CCM_EVENT_READ  0x01
#define CCM_EVENT_WRITE 0x02
#define CCM_EVENT_RDWR  0x03
#define CCM_EVENT_PRI   0x04
#define CCM_EVENT_RDH   0x08
#define CCM_EVENT_HUP   0x10
#define CCM_EVENT_ERR   0x20
#define CCM_EVENT_INT   0x40

#define CCM_EVENT_FLAG_ADDED   0x01
#define CCM_EVENT_FLAG_LISTEN  0x02
#define CCM_EVENT_FLAG_CONNECT 0x04

struct ccioevent {
  struct ccsmplnode node;
  handle_int fd;
  umedit_int udata;
  ushort_int masks;
  ushort_int flags;
};

struct ccionfmgr {
  CCPLAT_IMPL_SIZE(CC_IONFMGR_BYTES);
};

CORE_API nauty_bool ccionfmgr_init(struct ccionfmgr* self);
CORE_API void ccionfmgr_free(struct ccionfmgr* self);
CORE_API nauty_bool ccionfmgr_add(struct ccionfmgr* self, struct ccioevent* event);
CORE_API nauty_bool ccionfmgr_mod(struct ccionfmgr* self, struct ccioevent* event);
CORE_API nauty_bool ccionfmgr_del(struct ccionfmgr* self, struct ccioevent* event);
CORE_API int ccionfmgr_wait(struct ccionfmgr* self, void (*cb)(struct ccioevent*));
CORE_API int ccionfmgr_trywait(struct ccionfmgr* self, void (*cb)(struct ccioevent*));
CORE_API int ccionfmgr_timedwait(struct ccionfmgr* self, int ms, void (*cb)(struct ccioevent*));
CORE_API int ccionfmgr_wakeup(struct ccionfmgr* self);


CORE_API void ccplationftest();

#endif /* CCLIB_IONOTIFY_H_ */

