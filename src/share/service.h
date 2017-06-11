#ifndef CCLIB_SERVICE_H_
#define CCLIB_SERVICE_H_
#include "thatcore.h"
#include "luacapi.h"

#define CCM_MESSAGE_CONNIND 0x10
#define CCM_MESSAGE_CONNRSP 0x11
#define CCM_MESSAGE_IOEVENT 0x12
#define CCM_SERVICE_MASTERID (0)

typedef union {
  ccmedit_int s32;
  ccnauty_int s64;
  ccmedit_uint u32;
  ccnauty_uint u64;
  cchandle_int fd;
  void* ptr;
} ccmsgdata;

typedef struct ccmessage {
  struct ccsmplnode node;
  void* extra; /* dont move this field */
  umedit_int srcid;
  umedit_int dstid;
  umedit_int type;
  umedit_int flag;
  ccmsgdata data;
} ccmessage;

typedef struct ccthread ccthread;
CORE_API void ccthread_init(ccthread* self);
CORE_API void ccthread_free(ccthread* self);
CORE_API int ccthread_join(ccthread* self);
CORE_API void ccthread_sleep(uright_int us);
CORE_API void ccthread_exit();
CORE_API int startmainthread(int (*start)());
CORE_API int startmainthreadcv(int (*start)(), int argc, char** argv);
CORE_API nauty_bool ccthread_start(ccthread* self, int (*start)());
CORE_API ccthread* ccthread_getself();
CORE_API ccthread* ccthread_getmaster();
CORE_API ccstring* ccthread_getdefstr();

typedef struct ccbuffer {
  struct ccsmplnode node;
  umedit_int maxlimit;
  umedit_int capacity;
  umedit_int size;
  nauty_byte a[4];
} ccbuffer;

CORE_API void ccbuffer_ensurecapacity(ccbuffer** self, ccnauty_int size);
CORE_API void ccbuffer_ensuresizeremain(ccbuffer** self, ccnauty_int remainsize);
CORE_API ccbuffer* ccnewbuffer(ccthread* thread, umedit_int maxlimit);
CORE_API void ccfreebuffer(ccthread* thread, ccbuffer* p);


typedef struct ccservice ccservice;
CORE_API ccservice* ccservice_new(int (*entry)(ccservice*, ccmessage*));
CORE_API void* ccservice_setdata(ccservice* self, void* udata);
CORE_API void* ccservice_allocdata(ccservice* self, int bytes);
CORE_API void* ccservice_getdata(ccservice* self);
CORE_API void ccservice_setevent(ccservice* self, cchandle_int fd, ccshort_uint masks, ccshort_uint flags);
CORE_API void ccservice_start(ccservice* self);
CORE_API void ccservice_stop(ccservice* self);
CORE_API int ccservice_resume(ccservice* self, int (*func)(struct ccstate*));
CORE_API int ccservice_yield(ccservice* self, int (*kfunc)(struct ccstate*));

CORE_API ccthread* ccservice_belong(ccservice* self);
CORE_API cchandle_int ccservice_eventfd(ccservice* self);

CORE_API void ccmaster_dispatch_msg();
CORE_API void ccworker_handle_msg();


CORE_API ccmessage* message_create(umedit_int type, umedit_int flag);
CORE_API void* message_set_specific(ccmessage* self, void* data);
CORE_API void* message_set_allocated_specific(ccmessage* self, int bytes);
CORE_API void service_send_message(ccstate* state, umedit_int destid, ccmessage* msg);
CORE_API void service_send_message_sp(ccstate* state, umedit_int destid, umedit_int type, void* static_ptr);
CORE_API void service_send_message_um(ccstate* state, umedit_int destid, umedit_int type, umedit_int data);
CORE_API void service_send_message_fd(ccstate* state, umedit_int destid, umedit_int type, handle_int fd);

CORE_API ccservice* service_new(int (*entry)(ccservice*, ccmessage*));
CORE_API void* service_set_specific(ccservice* service, void* udata);
CORE_API void* service_set_allocated_specific(ccservice* service, int bytes);
CORE_API void service_set_listen(ccservice* service, handle_int fd, ushort_int masks, ushort_int flags);
CORE_API void service_start_run(ccservice* service);


CORE_API void* service_get_specific(ccstate* state);
CORE_API ccmessage* service_get_message(ccstate* state);
CORE_API handle_int service_get_eventfd(ccstate* state);
CORE_API void service_listen_event(ccstate* state, handle_int fd, ushort_int masks, ushort_int flags);
CORE_API void service_remove_listen(ccstate* state);
CORE_API void service_yield(ccstate* state, int (*kfunc)(ccstate*));
CORE_API void service_run_completed(ccstate* state);


#endif /* CCLIB_SERVICE_H_ */

