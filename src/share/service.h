#ifndef l_service_lib_h
#define l_service_lib_h
#include "thatcore.h"
#include "luacapi.h"

#define L_MESSAGE_CONNIND 0x10
#define L_MESSAGE_CONNRSP 0x11
#define L_MESSAGE_IOEVENT 0x12
#define L_SERVICE_MASTERID (0)

typedef union {
  l_medit s32;
  l_integer s64;
  l_umedit u32;
  l_uinteger u64;
  l_handle fd;
  void* ptr;
} l_msgdata;

typedef struct l_message {
  L_COMMON_BUFHEAD
  void* extra; /* dont move this field */
  l_umedit srcid;
  l_umedit dstid;
  l_umedit type;
  l_umedit flag;
  l_msgdata data;
} l_message;

typedef struct l_service {
  /* shared with master */
  l_linknode node;
  l_smplnode tlink;
  l_squeue rxmq;
  l_ioevent* event;
  l_ushort outflag;
  /* thread own use */
  l_ushort flag;
  l_umedit svid;
  l_thread* belong; /* set once when init */
  l_state* co;
  int (*entry)(l_service*, l_message*);
  void* udata;
} l_service;

l_extern void l_thread_init(l_thread* self);
l_extern void l_thread_free(l_thread* self);
l_extern int l_thread_join(l_thread* self);
l_extern void l_thread_sleep(l_integer us);
l_extern void l_thread_exit();
l_extern int startmainthread(int (*start)());
l_extern int startmainthreadcv(int (*start)(), int argc, char** argv);
l_extern int l_thread_start(l_thread* self, int (*start)());
l_extern l_logger* l_thread_get_logger();
l_extern l_thread* l_thread_self();
l_extern l_thread* l_thread_master();
l_extern l_string* l_thread_defstr();

l_extern l_service* l_service_new(int (*entry)(l_service*, l_message*));
l_extern void* l_service_set_data(l_service* self, void* udata);
l_extern void* l_service_alloc_data(l_service* self, int bytes);
l_extern void* l_service_data(l_service* self);
l_extern void l_service_set_event(l_service* self, l_handle fd, l_ushort masks, l_ushort flags);
l_extern void l_service_start(l_service* self);
l_extern void l_service_stop(l_service* self);
l_extern int l_service_resume(l_service* self, int (*func)(l_state*));
l_extern int l_service_yield(l_service* self, int (*kfunc)(l_state*));

l_extern l_thread* l_service_belong(l_service* self);
l_extern l_handle l_service_eventfd(l_service* self);

l_extern void l_master_dispatch_msg();
l_extern void l_worker_handle_msg();

l_extern l_message* message_create(l_umedit type, l_umedit flag);
l_extern void* message_set_specific(l_message* self, void* data);
l_extern void* message_set_allocated_specific(l_message* self, int bytes);
l_extern void service_send_message(l_state* state, l_umedit destid, l_message* msg);
l_extern void service_send_message_sp(l_state* state, l_umedit destid, l_umedit type, void* static_ptr);
l_extern void service_send_message_um(l_state* state, l_umedit destid, l_umedit type, l_umedit data);
l_extern void service_send_message_fd(l_state* state, l_umedit destid, l_umedit type, l_handle fd);

l_extern l_service* service_new(int (*entry)(l_service*, l_message*));
l_extern void* service_set_specific(l_service* service, void* udata);
l_extern void* service_set_allocated_specific(l_service* service, int bytes);
l_extern void service_set_listen(l_service* service, l_handle fd, l_ushort masks, l_ushort flags);
l_extern void service_start_run(l_service* service);

l_extern void* service_get_specific(l_state* state);
l_extern l_message* service_get_message(l_state* state);
l_extern l_handle service_get_eventfd(l_state* state);
l_extern void service_listen_event(l_state* state, l_handle fd, l_ushort masks, l_ushort flags);
l_extern void service_remove_listen(l_state* state);
l_extern void service_yield(l_state* state, int (*kfunc)(l_state*));
l_extern void service_run_completed(l_state* state);

#endif /* l_service_lib_h */

