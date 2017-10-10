#ifndef lucy_core_master_h
#define lucy_core_master_h
#include "core/base.h"

typedef struct lua_State lua_State;
typedef struct l_service l_service;
typedef struct l_thread l_thread;

L_GLOBAL l_thrkey l_thrkey_g;
L_THREAD_LOCAL_DECL(l_thread* l_the_thread);

L_INLINE l_thread*
l_thread_self()
{
#if defined(L_THREAD_LOCAL_SUPPORTED)
  return l_the_thread;
#else
  return (l_thread*)l_thrkey_getData(&l_thrkey_g);
#endif
}

L_EXTERN int l_thread_start(l_thread* self, int (*start)());
L_EXTERN int l_thread_join(l_thread* self);
L_EXTERN void l_thread_exit();
L_EXTERN l_thread* l_thread_master();
L_EXTERN lua_State* l_thread_luaState();

#define L_MESSAGE_START_SERVICE 0x01
#define L_MESSAGE_START_SRVCRSP 0x02
#define L_MESSAGE_CLOSE_SERVICE 0x03
#define L_MESSAGE_CLOSE_SRVCRSP 0x04
#define L_MESSAGE_CLOSE_EVENTFD 0x05
#define L_MESSAGE_BOOTSTRAP 0x06
#define L_MESSAGE_MASTER_EXIT 0x07
#define L_MESSAGE_WORKER_EXIT 0x08
#define L_MESSAGE_WORKER_EXITRSP 0x09
#define L_MESSAGE_IOEVENT 0x7d
#define L_MESSAGE_CONNRSP 0x7e
#define L_MESSAGE_CONNIND 0x7f
#define L_MIN_USER_MSGID  0x80

typedef struct {
  L_COMMON_BUFHEAD
  l_service* srvc;
  l_umedit dstid;
  l_umedit type;
} l_message;

typedef struct {
  l_message head;
  l_handle fd;
  l_umedit masks;
  l_sockaddr remote;
} l_connind_message;

typedef struct {
  l_message head;
  l_handle fd;
  l_umedit masks;
} l_ioevent_message;

typedef struct {
  l_message head;
  l_umedit svid;
  l_handle fd;
} l_service_message;

typedef struct {
  l_message head;
  int (*bootstrap)();
} l_bootstrap_message;

typedef struct {
  l_message head;
  l_handle fd;
} l_message_fd;

typedef struct {
  l_message head;
  void* ptr;
} l_message_ptr;

typedef struct {
  l_message head;
  void* ptr;
  l_int n;
} l_message_ptrn;

typedef struct {
  l_message head;
  l_umedit u32;
} l_message_u32;

typedef struct {
  l_message head;
  l_medit s32;
} l_message_s32;

typedef struct {
  l_message head;
  l_ulong u64;
} l_message_u64;

typedef struct {
  l_message head;
  l_long s64;
} l_message_s64;

L_EXTERN l_message* l_message_create(l_thread* thread, l_umedit type, l_int size);
L_EXTERN void l_message_free(l_thread* thread, l_message* msg);
L_EXTERN void l_message_freeQueue(l_thread* thread, l_squeue* mq);
L_EXTERN void l_message_send(l_thread* thread, l_umedit destid, l_message* msg);
L_EXTERN void l_message_sendType(l_thread* thread, l_umedit destid, l_umedit type);
L_EXTERN void l_message_sendHandle(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd);
L_EXTERN void l_message_sendPtr(l_thread* thread, l_umedit destid, l_umedit type, void* ptr);
L_EXTERN void l_message_sendU32(l_thread* thread, l_umedit destid, l_umedit type, l_umedit u32);
L_EXTERN void l_message_sendU64(l_thread* thread, l_umedit destid, l_umedit type, l_ulong u64);
L_EXTERN void l_message_sendS32(l_thread* thread, l_umedit destid, l_umedit type, l_medit s32);
L_EXTERN void l_message_sendS64(l_thread* thread, l_umedit destid, l_umedit type, l_long s64);
L_EXTERN void l_message_sendEvent(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd, l_umedit masks);
L_EXTERN void l_message_sendService(l_thread* thread, l_umedit destid, l_umedit type, l_umedit svid, l_handle fd);
L_EXTERN void l_message_sendBootstrap(l_thread* thread, int (*bootstrap)());

#define L_SERVICE_MASTER_ID (0) /* worker's default svid is its index (16-bit) */
#define L_SERVICE_BOOTSTRAP (0xffff+1)

L_EXTERN l_service* l_create_service(l_int size, int (*entry)(l_service*, l_message*), void (*destroy)(l_service*));
L_EXTERN l_service* l_create_service_in_same_thread(l_int size, int (*entry)(l_service*, l_message*), void (*destroy)(l_service*));
L_EXTERN int l_free_unstarted_service(l_service* srvc);
L_EXTERN void l_start_service(l_service* srvc);
L_EXTERN void l_start_listener_service(l_service* srvc, l_handle sock);
L_EXTERN void l_start_initiator_service(l_service* srvc, l_handle sock);
L_EXTERN void l_start_receiver_service(l_service* srvc, l_handle sock);
L_EXTERN void l_close_service(l_service* srvc);
L_EXTERN void l_close_event(l_service* srvc);
L_EXTERN int startmainthread(int (*start)());
L_EXTERN int startmainthreadcv(int (*start)(), int argc, char** argv);
L_EXTERN void l_master_exit();

#endif /* lucy_core_master_h */

