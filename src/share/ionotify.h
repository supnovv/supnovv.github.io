#ifndef CCLIB_IONOTIFY_H_
#define CCLIB_IONOTIFY_H_
#include "thatcore.h"

#define CCIONFRD  0x01
#define CCIONFWR  0x02
#define CCIONFRW  0x03
#define CCIONFPRI 0x04
#define CCIONFRDH 0x08
#define CCIONFHUP 0x10
#define CCIONFERR 0x20
#define CCIONFINT 0x40

#define CCSOCK_LISTEN  0x1000
#define CCSOCK_CONNECT 0x2000

struct ccionfevt {
  struct ccsmplnode node;
  handle_int fd;
  umedit_int udata;
  umedit_int masks;
};

struct ccionfmgr {
  CCPLAT_IMPL_SIZE(CC_IONFMGR_BYTES);
};

CORE_API nauty_bool ccionfmgr_init(struct ccionfmgr* self);
CORE_API void ccionfmgr_free(struct ccionfmgr* self);
CORE_API nauty_bool ccionfmgr_add(struct ccionfmgr* self, struct ccionfevt* event);
CORE_API nauty_bool ccionfmgr_mod(struct ccionfmgr* self, struct ccionfevt* event);
CORE_API nauty_bool ccionfmgr_del(struct ccionfmgr* self, struct ccionfevt* event);
CORE_API int ccionfmgr_wait(struct ccionfmgr* self, void (*cb)(struct ccionfevt*));
CORE_API int ccionfmgr_trywait(struct ccionfmgr* self, void (*cb)(struct ccionfevt*));
CORE_API int ccionfmgr_timedwait(struct ccionfmgr* self, int ms, void (*cb)(struct ccionfevt*));
CORE_API int ccionfmgr_wakeup(struct ccionfmgr* self);


CORE_API void ccplationftest();

#endif /* CCLIB_IONOTIFY_H_ */

