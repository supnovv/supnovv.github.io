#ifndef lucy_core_master_h
#define lucy_core_master_h
#include "core/base.h"
#include "core/queue.h"
#include "core/string.h"
#include "core/fileop.h"
#include "core/socket.h"
#include "core/thread.h"

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

L_EXTERN int l_thread_join(l_thread* self);
L_EXTERN void l_thread_exit();
L_EXTERN l_thread* l_thread_master();
L_EXTERN lua_State* l_thread_luaState();

typedef struct {
  l_smplnode node;
  l_int bsize;
} L_BUFHEAD;

typedef struct {
  L_BUFHEAD HEAD;
  l_ulong dest;
  l_umedit type;
  l_umedit data;
  l_ulong extra;
} l_message;

typedef struct {
  l_message head;
  l_filedesc fd;
  l_umedit masks;
  l_sockaddr remote;
} l_connind_message;

typedef struct {
  l_message head;
  l_filedesc fd;
  l_umedit masks;
} l_ioevent_message;

typedef struct {
  l_message head;
  l_umedit svid;
  l_filedesc sock;
} l_service_message;

typedef struct {
  l_message head;
  int (*bootstrap)();
} l_bootstrap_message;

typedef struct {
  l_message head;
  l_filedesc fd;
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
L_EXTERN void l_message_sendHandle(l_thread* thread, l_umedit destid, l_umedit type, l_filedesc fd);
L_EXTERN void l_message_sendPtr(l_thread* thread, l_umedit destid, l_umedit type, void* ptr);
L_EXTERN void l_message_sendU32(l_thread* thread, l_umedit destid, l_umedit type, l_umedit u32);
L_EXTERN void l_message_sendU64(l_thread* thread, l_umedit destid, l_umedit type, l_ulong u64);
L_EXTERN void l_message_sendS32(l_thread* thread, l_umedit destid, l_umedit type, l_medit s32);
L_EXTERN void l_message_sendS64(l_thread* thread, l_umedit destid, l_umedit type, l_long s64);
L_EXTERN void l_message_sendEvent(l_thread* thread, l_umedit destid, l_umedit type, l_filedesc fd, l_umedit masks);
L_EXTERN void l_message_sendService(l_thread* thread, l_umedit destid, l_umedit type, l_umedit svid, l_filedesc fd);
L_EXTERN void l_message_sendBootstrap(l_thread* thread, int (*bootstrap)());

L_EXTERN l_service* l_service_create(l_int size, int (*entry)(l_service*, l_message*), void (*destroy)(l_service*));
L_EXTERN int l_service_free(l_service* srvc);
L_EXTERN void l_service_start(l_service* srvc);
L_EXTERN void l_service_startEx(l_service* srvc, l_thread* thread);
L_EXTERN void l_start_listener_service(l_service* srvc, l_filedesc sock);
L_EXTERN void l_start_initiator_service(l_service* srvc, l_filedesc sock);
L_EXTERN void l_start_receiver_service(l_service* srvc, l_filedesc sock);
L_EXTERN void l_close_service(l_service* srvc);
L_EXTERN void l_close_event(l_service* srvc);
L_EXTERN int startmainthread(int (*start)());
L_EXTERN int startmainthreadcv(int (*start)(), int argc, char** argv);
L_EXTERN void l_master_exit();

#endif /* lucy_core_master_h */

