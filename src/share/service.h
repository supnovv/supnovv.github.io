#ifndef CCLIB_SERVICE_H_
#define CCLIB_SERVICE_H_
#include "thatcore.h"
#include "luacapi.h"

#define MESSAGE_CONNIND 0x10
#define MESSAGE_CONNRSP 0x11
#define MESSAGE_IOEVENT 0x12
#define ROBOT_ID_MASTER (0)

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

struct ccbuffer {
  struct ccsmplnode node;
  umedit_int maxlimit;
  umedit_int capacity;
  umedit_int size;
  nauty_byte a[4];
};

CORE_API void ccbuffer_ensurecapacity(struct ccbuffer** self, ccnauty_int size);
CORE_API void ccbuffer_ensuresizeremain(struct ccbuffer** self, ccnauty_int remainsize);
CORE_API struct ccbuffer* ccnewbuffer(struct ccthread* thread, umedit_int maxlimit);
CORE_API void ccfreebuffer(struct ccthread* thread, struct ccbuffer* p);


struct ccrobot;
CORE_API void ccmaster_dispatch_msg();
CORE_API void ccworker_handle_msg();


CORE_API struct ccmessage* message_create(umedit_int type, umedit_int flag);
CORE_API void* message_set_specific(struct ccmessage* self, void* data);
CORE_API void* message_set_allocated_specific(struct ccmessage* self, int bytes);
CORE_API void robot_send_message(struct ccstate* state, umedit_int destid, struct ccmessage* msg);
CORE_API void robot_send_message_sp(struct ccstate* state, umedit_int destid, umedit_int type, void* static_ptr);
CORE_API void robot_send_message_um(struct ccstate* state, umedit_int destid, umedit_int type, umedit_int data);
CORE_API void robot_send_message_fd(struct ccstate* state, umedit_int destid, umedit_int type, handle_int fd);

CORE_API struct ccrobot* robot_new(int (*entry)(struct ccrobot*, struct ccmessage*));
CORE_API void* robot_set_specific(struct ccrobot* robot, void* udata);
CORE_API void* robot_set_allocated_specific(struct ccrobot* robot, int bytes);
CORE_API void robot_set_listen(struct ccrobot* robot, handle_int fd, ushort_int masks, ushort_int flags);
CORE_API void robot_start_run(struct ccrobot* robot);


CORE_API void* robot_get_specific(struct ccstate* state);
CORE_API struct ccmessage* robot_get_message(struct ccstate* state);
CORE_API handle_int robot_get_eventfd(struct ccstate* state);
CORE_API void robot_listen_event(struct ccstate* state, handle_int fd, ushort_int masks, ushort_int flags);
CORE_API void robot_remove_listen(struct ccstate* state);
CORE_API void robot_yield(struct ccstate* state, int (*kfunc)(struct ccstate*));
CORE_API void robot_run_completed(struct ccstate* state);




#endif /* CCLIB_SERVICE_H_ */

