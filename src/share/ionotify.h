#ifndef CCLIB_IONOTIFY_H_
#define CCLIB_IONOTIFY_H_
#include "thatcore.h"

#define CCIONFRD  0x0001
#define CCIONFWR  0x0002
#define CCIONFRW  0x0003
#define CCIONFPRI 0x0004
#define CCIONFRDH 0x0008
#define CCIONFHUP 0x0010
#define CCIONFERR 0x0020
#define CCIONFINT 0x0040

#define CCIOFLAG_ADDED 0x01
#define CCIOFLAG_LISTEN 0x10
#define CCIOFLAG_CONNECT 0x20

#define IOEVENT_READ  0x01
#define IOEVENT_WRITE 0x02
#define IOEVENT_RDWR  0x03
#define IOEVENT_PRI   0x04
#define IOEVENT_RDH   0x08
#define IOEVENT_HUP   0x10
#define IOEVENT_ERR   0x20
#define IOEVENT_INT   0x40

#define IOEVENT_FLAG_ADDED   0x01
#define IOEVENT_FLAG_LISTEN  0x02
#define IOEVENT_FLAG_CONNECT 0x04

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

