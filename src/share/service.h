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

#define ROBOT_MASTER_ID (0)

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

struct ccrobot;
CORE_API struct ccionfmgr* ccgetionfmgr();
CORE_API struct ccstate* ccrobot_getstate(struct ccrobot* self);
CORE_API struct ccmessage* ccnewmessage(umedit_int destsvid, umedit_int type);
CORE_API struct ccmessage* ccnewmessage_allocated(umedit_int destsvid, umedit_int type, void* ptr);
CORE_API void ccrobot_sendmsg(struct ccrobot* self, struct ccmessage* msg);
CORE_API void ccrobot_sendmsgtomaster(struct ccrobot* self, struct ccmessage* msg);
CORE_API void ccrobot_sendtomaster(struct ccrobot* self, umedit_int type);
CORE_API void ccmaster_dispatch_msg();
CORE_API void ccworker_handle_msg();


CORE_API struct ccmessage* message_create(umedit_int type, umedit_int flag);
CORE_API void* message_set_specific(struct ccmessage* self, void* data);
CORE_API void* message_set_allocated_specific(struct ccmessage* self, int bytes);
CORE_API void robot_send_message(struct ccstate* state, struct ccmessage* msg, umedit_int destid);

CORE_API struct ccrobot* robot_create_new(int (*func)(struct ccstate*), int yieldable);
CORE_API struct ccrobot* robot_create_from(struct ccstate* state, int (*func)(struct ccstate*), int yieldable);
CORE_API void* robot_set_specific(struct ccrobot* robot, void* udata);
CORE_API void* robot_set_allocated_specific(struct ccrobot* robot, int bytes);
CORE_API void robot_set_event(struct ccrobot* robot, handle_int fd, ushort_int masks, ushort_int flags);
CORE_API void robot_start_run(struct ccrobot* robot);


CORE_API void* robot_get_specific(struct ccstate* state);
CORE_API struct ccmessage* robot_get_message(struct ccstate* state);
CORE_API void robot_listen_event(struct ccstate* state, handle_int fd, ushort_int masks, ushort_int flags);
CORE_API void robot_remove_event(struct ccstate* state);
CORE_API void robot_resume(struct ccstate* state, const char* robot);
CORE_API void robot_yield(struct ccstate* state, int (*kfunc)(struct ccstate*));
CORE_API void robot_run_completed(struct ccstate* state);




#endif /* CCLIB_SERVICE_H_ */

