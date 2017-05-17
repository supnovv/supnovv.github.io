#ifndef CCLIB_SERVICE_H_
#define CCLIB_SERVICE_H_
#include "thatcore.h"

#define CCMSGID_ADDSRVC 0x01
#define CCMSGID_DELSRVC 0x02
#define CCMSGID_IOEVENT 0x10
#define CCMSGID_CONNIND 0x11
#define CCMSGID_CONNRSP 0x12
#define CCMSGFLAG_FREEPTR 0x01

#define MESSAGE_ID_CONNIND 0x11
#define MESSAGE_ID_CONNRSP 0x12
#define MESSAGE_ID_IOEVENT 0x10

#define ROBOT_DONE     0x00
#define ROBOT_CONTINUE 0x01

#define ROBOT_FLAG_FREEDATA 0x01

#if 0
void robot_set_specific(struct ccrobot* self, void* data);
void robot_set_allocated_specific(struct ccrobot* self, void* data);
#endif

union ccmsgdata {
  handle_int fd;
  uoctet_int ub;
  soctet_int sb;
  ushort_int us;
  sshort_int ss;
  umedit_int um;
  smedit_int sm;
  uright_int ui;
  sright_int si;
  uoctet_int a_ub[8];
  soctet_int a_sb[8];
  ushort_int a_us[4];
  sshort_int a_ss[4];
  umedit_int a_um[2];
  smedit_int a_sm[2];
  void* ptr;
};

struct ccmessage {
  struct ccsmplnode node;
  void* extra; /* dont move this field */
  umedit_int srcid;
  umedit_int dstid;
  umedit_int type;
  umedit_int flag;
  union ccmsgdata data;
};

struct ccthread;
CORE_API void ccthread_init(struct ccthread* self);
CORE_API void ccthread_free(struct ccthread* self);
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
CORE_API struct ccstate* ccservice_getstate(struct ccservice* self);
CORE_API struct ccmessage* ccnewmessage(umedit_int destsvid, umedit_int type);
CORE_API struct ccmessage* ccnewmessage_allocated(umedit_int destsvid, umedit_int type, void* ptr);
CORE_API void ccservice_sendmsg(struct ccservice* self, struct ccmessage* msg);
CORE_API void ccservice_sendmsgtomaster(struct ccservice* self, struct ccmessage* msg);
CORE_API void ccservice_sendtomaster(struct ccservice* self, umedit_int type);
CORE_API void ccmaster_dispatch_msg();
CORE_API void ccworker_handle_msg();

#endif /* CCLIB_SERVICE_H_ */

