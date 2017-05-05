#ifndef CCLIB_IONOTIFY_H_
#define CCLIB_IONOTIFY_H_
#include "thatcore.h"

#define cchandle _medit

#define CCIONFRD  0x01
#define CCIONFWR  0x02
#define CCIONFRW  0x03
#define CCIONFPRI 0x04
#define CCIONFRDH 0x08
#define CCIONFHUP 0x10
#define CCIONFERR 0x20
#define CCIONFINT 0x40

struct ccionfevt {
  cchandle hdl;
  umedit masks;
  umedit udata;
};

struct ccionfmgr {
  union cceight a[32];
};

CORE_API bool ccionfmgr_init(struct ccionfmgr* self);
CORE_API void ccionfmgr_free(struct ccionfmgr* self);
CORE_API bool ccionfmgr_add(struct ccionfmgr* self, struct ccionfevt* event);
CORE_API bool ccionfmgr_mod(struct ccionfmgr* self, struct ccionfevt* event);
CORE_API bool ccionfmgr_del(struct ccionfmgr* self, struct ccionfevt* event);
CORE_API void ccionfmgr_wait(struct ccionfmgr* self, void (*cb)(struct ccionfevt*));
CORE_API void ccionfmgr_trywait(struct ccionfmgr* self, void (*cb)(struct ccionfevt*));
CORE_API void ccionfmgr_timedwait(struct ccionfmgr* self, int ms, void (*cb)(struct ccionfevt*));

#endif /* CCLIB_IONOTIFY_H_ */

