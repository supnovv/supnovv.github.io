#ifndef l_core_service_h
#define l_core_service_h
#include "core/base.h"
#include "core/queue.h"

#define L_MSGID_SERVICE_START 0x01
#define L_MSGID_SERVICE_CLOSE 0x02

typedef struct lua_State lua_State;
typedef struct l_service l_service;
typedef struct l_thread l_thread;

typedef struct {
  l_smplnode node;
  l_int bsize;
} L_BUFHEAD;

typedef struct {
  L_BUFHEAD HEAD;
  l_ulong dest;
  l_umedit msgid;
  l_umedit data;
  l_ulong extra;
} l_message;

typedef struct {
  l_message head;
  l_byte addr[16];
} l_connind_message;

L_INLINE l_ushort
l_msg_dest_tidx(l_message* msg)
{
  return (l_ushort)(msg->dest >> 48);
}

L_INLINE l_ushort
l_msg_dest_cust(l_message* msg)
{
  return (l_ushort)((msg->dest & 0xffff00000000ull) >> 32);
}

L_INLINE l_umedit
l_msg_dest_svid(l_message* msg)
{
  return (l_umedit)(msg->dest & 0xffffffff);
}

L_INLINE l_umedit
l_msg_dest_high(l_message* msg)
{
  return (l_umedit)(msg->dest >> 32);
}

L_INLINE l_ulong
l_msg_castptr(void* p)
{
  return (l_ulong)(l_uint)p;
}

L_INLINE void*
l_msg_getptr(l_message* msg)
{
  return (void*)(l_uint)msg->extra;
}

L_INLINE l_ulong
l_msg_castfd(l_filedesc fd)
{
#if defined(l_plat_windows)
  return (l_ulong)fd.winfd;
#else
  return (l_ulong)fd.unifd;
#endif
}

L_INLINE l_filedesc
l_msg_getfd(l_message* msg)
{
  l_filedesc fd;
#if defined(l_plat_windows)
  fd.winfd = (HANDLE)msg->extra;
#else
  fd.unifd = (int)msg->extra;
#endif
  return fd;
}

L_INLINE l_filedesc
l_msg_getfdFrom(l_ulong a)
{
  l_filedesc fd;
#if defined(l_plat_windows)
  fd.winfd = (HANDLE)a;
#else
  fd.unifd = (int)a;
#endif
  return fd;
}

L_INLINE l_ulong
l_msg_castfunc(int (*func)())
{
  return (l_ulong)(l_uint)(void*)func;
}

L_INLINE l_ushort
l_connind_getFamily(l_connind_message* msg)
{
  return (l_ushort)(msg->head.data >> 16);
}

L_INLINE l_ushort
l_connind_getPort(l_connind_message* msg)
{
  return (l_ushort)(msg->head.data & 0xffff);
}

L_INLINE l_filedesc
l_connind_getSock(l_connind_message* msg)
{
  return l_msg_getfd(&msg->head);
}

L_EXTERN l_message* l_message_create(l_int size, l_thread* thread);
L_EXTERN void l_message_free(l_message* msg, l_thread* thread);
L_EXTERN void l_message_freeQueue(l_squeue* mq, l_thread* thread);
L_EXTERN void l_message_send(l_thread* from, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64, l_message* msg);
L_EXTERN void l_message_sendData(l_thread* from, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64);

/* if custom service has any extra resource need to free,
the only chance is to handle L_MSGID_SERVICE_CLOSE message. */

typedef struct l_service {
  L_BUFHEAD HEAD;
  l_filedesc evfd; /* guard by svmtx */
  l_ushort evmk; /* guard by svmtx */
  l_ushort flags; /* shared flags stop_rx_msg service is closing, guard by svmtx */
  /* thread own use */
  l_thread* thread; /* only set once when init, so can freely access it */
  int (*entry)(l_service*, l_message*); /* service entry function */
  l_ulong svid; /* only set once when init, so can freely access it */
  l_umedit flagw; /* only accessed by a worker */
  /* coroutine */
  int coref;
  lua_State* co;
  int (*func)(l_service*);
  int (*kfunc)(l_service*);
} l_service;

#define L_SERVICE_CREATE(name) (name*)l_service_create(sizeof(name), name##_proc)
#define L_SERVICE_CREATEFROM(srvc, name) (name*)l_service_createFrom((srvc), sizeof(name), name##_proc)

L_EXTERN l_service* l_service_create(l_int size, int (*entry)(l_service*, l_message*));
L_EXTERN l_service* l_service_createFrom(l_service* from, l_int size, int (*entry)(l_service*, l_message*));
L_EXTERN l_service* l_service_setListen(l_service* srvc, l_filedesc fd);
L_EXTERN l_service* l_service_setConnect(l_service* srvc, l_filedesc fd);
L_EXTERN l_service* l_service_setEvent(l_service* srvc, l_filedesc fd, l_ushort masks);
L_EXTERN l_ulong l_service_id(l_service* srvc);
L_EXTERN void l_service_start(l_service* srvc);
L_EXTERN void l_service_startEx(l_service* srvc, l_thread* thread);
L_EXTERN void l_service_delEvent(l_service* srvc);
L_EXTERN void l_service_modEvent(l_service* srvc, l_filedesc fd, l_ushort masks);
L_EXTERN void l_service_modListen(l_service* srvc, l_filedesc fd);
L_EXTERN void l_service_modConnect(l_service* srvc, l_filedesc fd);
L_EXTERN void l_service_close(l_service* srvc);
L_EXTERN int l_service_initState(l_service* srvc);
L_EXTERN void l_service_freeState(l_service* srvc);
L_EXTERN int l_service_isYield(l_service* srvc);
L_EXTERN int l_service_setResume(l_service* srvc, int (*func)(l_service*));
L_EXTERN int l_service_resume(l_service* srvc);
L_EXTERN int l_service_yield(l_service* srvc, int (*kfunc)(l_service*));
L_EXTERN int l_service_yieldWith(l_service* srvc, int (*kfunc)(l_service*), int code);

/* master service */

L_EXTERN int startmainthread(int (*start)());
L_EXTERN int startmainthreadcv(int (*start)(), int argc, char** argv);
L_EXTERN void l_master_exit();
L_EXTERN void l_master_test();

#endif /* l_core_service_h */

