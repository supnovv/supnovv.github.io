#ifndef l_message_lib_h
#define l_message_lib_h
#include "thatcore.h"
#include "l_socket.h"

#define L_MESSAGE_IOEVENT 0x7d
#define L_MESSAGE_CONNRSP 0x7e
#define L_MESSAGE_CONNIND 0x7f
#define L_MIN_USER_MSGID  0x80

typedef struct l_message {
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
  l_handle fd;
} l_message_fd;

typedef struct {
  l_message head;
  void* ptr;
} l_message_ptr;

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

l_extern l_message* l_create_message(l_thread* thread, l_umedit type, l_int size);
l_extern void l_free_message(l_thread* thread, l_message* msg);
l_extern void l_free_message_queue(l_thread* thread, l_squeue* mq);
l_extern void l_send_message(l_thread* thread, l_umedit destid, l_message* msg);
l_extern void l_send_message_tp(l_thread* thread, l_umedit destid, l_umedit type);
l_extern void l_send_message_fd(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd);
l_extern void l_send_message_ptr(l_thread* thread, l_umedit destid, l_umedit type, void* ptr);
l_extern void l_send_message_u32(l_thread* thread, l_umedit destid, l_umedit type, l_umedit u32);
l_extern void l_send_message_u64(l_thread* thread, l_umedit destid, l_umedit type, l_ulong u64);
l_extern void l_send_message_s32(l_thread* thread, l_umedit destid, l_umedit type, l_medit s32);
l_extern void l_send_message_s64(l_thread* thread, l_umedit destid, l_umedit type, l_long s64);

l_extern void l_send_service_message(l_thread* thread, l_umedit destid, l_umedit type, l_umedit svid, l_handle fd);

#endif /* l_message_lib_h */

