#include "l_message.h"

l_message* l_create_message(l_thread* thread, l_umedit type, l_int size) {
  l_message* msg = 0;
  if (size < sizeof(l_message) || type <= L_MIN_USER_MSGID) return 0;
  msg = (l_message*)l_thread_pop_buffer(thread->frbf, size);
  msg->type = type;
  return msg;
}

void l_free_message(l_thread* thread, l_message* msg) {
  l_thread_push_buffer(thread->frbf, msg);
}

void l_send_message(l_thread* thread, l_umedit destid, l_message* msg) {
  msg->srvc = 0;
  msg->dstid = destid;
  if (destid == L_SERVICE_MASTER_ID) {
    l_squeue_push(thread->txms, &msg->node);
  } else {
    l_squeue_push(thread->txmq, &msg->node);
  }
}

void l_send_message_tp(l_thread* thread, l_umedit destid, l_umedit type) {
  l_message* msg = l_create_message(thread, type, sizeof(l_message));
  l_send_message(thread, destid, msg);
}

void l_send_message_fd(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd) {
  l_message_fd* msg = (l_message_fd*)l_create_message(thread, type, sizeof(l_message_fd));
  msg->fd = fd;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_ptr(l_thread* thread, l_umedit destid, l_umedit type, void* ptr) {
  l_message_ptr* msg = (l_message_ptr*)l_create_message(thread, type, sizeof(l_message_ptr));
  msg->ptr = ptr;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_s32(l_thread* thread, l_umedit destid, l_umedit type, l_medit s32) {
  l_message_s32* msg = (l_message_s32*)l_create_message(thread, type, sizeof(l_message_s32));
  msg->s32 = s32;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_u32(l_thread* thread, l_umedit destid, l_umedit type, l_umedit u32) {
  l_message_u32* msg = (l_message_u32*)l_create_message(thread, type, sizeof(l_message_u32));
  msg->u32 = u32;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_s64(l_thread* thread, l_umedit destid, l_umedit type, l_long s64) {
  l_message_s64* msg = (l_message_s64*)l_create_message(thread, type, sizeof(l_message_s64));
  msg->s64 = s64;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_u64(l_thread* thread, l_umedit destid, l_umedit type, l_ulong u64) {
  l_message_u64* msg = (l_message_u64*)l_create_message(thread, type, sizeof(l_message_u64));
  msg->u64 = u64;
  l_send_message(thread, destid, &msg->head);
}

