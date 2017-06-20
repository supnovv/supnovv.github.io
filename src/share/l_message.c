#include "thatcore.h"

l_message* l_create_message(l_thread* thread, l_umedit type, l_int size) {
  l_message* msg = 0;
  if (size < sizeof(l_message)) return 0;
  msg = (l_message*)l_thread_pop_buffer(thread, size);
  msg->type = type;
  return msg;
}

void l_free_message(l_thread* thread, l_message* msg) {
  l_thread_push_buffer(thread, msg);
}

void l_send_message(l_thread* thread, l_umedit destid, l_message* msg) {
  msg->extra = 0;
  msg->dstid = destid;
  if (destid == L_SERVICE_MASTER_ID) {
    l_squeue_push(&thread->txms, &msg->node);
  } else {
    l_squeue_push(&thread->txmq, &msg->node);
  }
}

void l_send_message_fd(l_service* service, l_umedit destid, l_umedit type, l_handle fd) {
  l_thread* thread = service->belong;
  l_message* msg = l_create_message(thread, type, 0);
  msg->data.fd = fd;
  l_send_message(service, destid, msg);
}

void l_send_message_ptr(l_service* service, l_umedit destid, l_umedit type, void* data_ptr) {
  l_thread* thread = service->belong;
  l_message* msg = l_create_message(thread, type, 0);
  msg->data.ptr = data_ptr;
  l_send_message(service, destid, msg);
}

void l_send_message_s32(l_service* service, l_umedit destid, l_umedit type, l_medit s32) {
  l_thread* thread = service->belong;
  l_message* msg = l_create_message(thread, type, 0);
  msg->data.s32 = s32;
  l_send_message(service, destid, msg);
}

void l_send_message_u32(l_service* service, l_umedit destid, l_umedit type, l_umedit u32) {
  l_thread* thread = service->belong;
  l_message* msg = l_create_message(thread, type, 0);
  msg->data.u32 = u32;
  l_send_message(service, destid, msg);
}

void l_send_message_s64(l_service* service, l_umedit destid, l_umedit type, l_long s64) {
  l_thread* thread = service->belong;
  l_message* msg = l_create_message(thread, type, 0);
  msg->data.s64 = s64;
  l_send_message(service, destid, msg);
}

void l_send_message_u64(l_service* service, l_umedit destid, l_umedit type, l_ulong u64) {
  l_thread* thread = service->belong;
  l_message* msg = l_create_message(thread, type, 0);
  msg->data.u64 = u64;
  l_send_message(service, destid, msg);
}

