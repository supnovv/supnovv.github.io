#ifndef CCLIB_SERVICE_H_
#define CCLIB_SERVICE_H_
#include "thatcore.h"

struct ccmsgnode {
  struct ccsmplnode node;
  void* extra; /* dont move this field */
  umedit_int srcid;
  umedit_int dstid;
  void* ptr;
  uright_int data;
};

struct ccthread;
CORE_API void ccthread_init(struct ccthread* self);
CORE_API int ccthread_join(struct ccthread* self);
CORE_API void ccthread_sleep(uright_int us);
CORE_API void ccthread_exit();
CORE_API int startmainthread(int (*start)());
CORE_API int startmainthreadcv(int (*start)(), int argc, char** argv);
CORE_API nauty_bool ccthread_start(struct ccthread* self, int (*start)());
CORE_API struct ccthread* ccthread_getself();
CORE_API struct ccthread* ccthread_getmaster();
CORE_API struct ccstring* ccthread_getdefstr();

struct ccservice;
CORE_API struct ccionfmgr* ccgetionfmgr();
CORE_API void ccservice_sendmsg(struct ccservice* self, umedit_int destsvid, sright_int data, void* ptr);
CORE_API void ccservice_sendtomaster(struct ccservice* self, sright_int data);

#endif /* CCLIB_SERVICE_H_ */

