#include "lucycore.h"
#include "net/socket.h"

#define L_SERVICE_SAMETHRD 0x01
#define L_SERVICE_IO_EVENT 0x02
#define L_SERVICE_CLOSING  0x04

void l_start_service(l_service* srvc) {
  l_thread* thread = srvc->thread;
  if (!(srvc->wflgs & L_SERVICE_SAMETHRD)) {
    srvc->thread = 0;
  }
  l_send_message_ptr(thread, L_SERVICE_MASTER_ID, L_MESSAGE_START_SERVICE, srvc);
}

static void ll_start_ioevent_service(l_service* srvc, l_handle sock, l_ushort masks, l_ushort flags) {
  l_ioevent* ioev = 0;

  if (!l_socket_is_open(sock)) {
    l_logw_1("sock %d", ld(sock));
    return l_start_service(srvc);
  }

  srvc->ioev = &srvc->event;
  srvc->wflgs |= L_SERVICE_IO_EVENT;
  srvc->mflgs |= L_SERVICE_IO_EVENT;

  ioev = srvc->ioev;
  ioev->fd = sock;
  ioev->udata = srvc->svid;
  ioev->masks = masks;
  ioev->flags = flags;
  l_start_service(srvc);
}

void l_start_listener_service(l_service* srvc, l_handle sock) {
  return ll_start_ioevent_service(srvc, sock, L_IOEVENT_READ, L_IOEVENT_FLAG_LISTEN);
}

void l_start_initiator_service(l_service* srvc, l_handle sock) {
  return ll_start_ioevent_service(srvc, sock, L_IOEVENT_RDWR, L_IOEVENT_FLAG_CONNECT);
}

void l_start_receiver_service(l_service* srvc, l_handle sock) {
  return ll_start_ioevent_service(srvc, sock, L_IOEVENT_RDWR, 0);
}

void l_close_service(l_service* srvc) {
  srvc->wflgs |= L_SERVICE_CLOSING;
}

void l_close_event(l_service* srvc) {
  l_thread* thread = srvc->thread;
  l_handle fd = -1;

  l_debug_assert(thread == l_thread_self());

  if (srvc->wflgs & L_SERVICE_IO_EVENT) {
    l_ioevent* ioev = 0;
    l_mutex* svmtx = thread->svmtx;

    l_mutex_lock(svmtx);
    ioev = srvc->ioev;
    /* store fd to send to mater to close */
    fd = ioev->fd;
    /* reset fd as closed */
    ioev->fd = -1;
    /* if thread still has io message in the pending queue,
    when to handle it, this message will be ignored due to
    no event attached to service from now */
    srvc->ioev = 0;
    l_mutex_unlock(svmtx);

    srvc->wflgs &= (~L_SERVICE_IO_EVENT);
  }

  if (fd == -1) return;
  l_send_service_message(thread, L_SERVICE_MASTER_ID, L_MESSAGE_CLOSE_EVENTFD, srvc->svid, fd);
}

int l_service_yield(l_service* self, int (*kfunc)(l_state*)) {
  return l_state_yield(self->co, kfunc);
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

void l_send_ioevent_message(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd, l_umedit masks) {
  l_ioevent_message* msg = (l_ioevent_message*)l_create_message(thread, type, sizeof(l_ioevent_message));
  msg->fd = fd;
  msg->masks = masks;
  l_send_message(thread, destid, &msg->head);
}

void l_send_service_message(l_thread* thread, l_umedit destid, l_umedit type, l_umedit svid, l_handle fd) {
  l_service_message* msg = (l_service_message*)l_create_message(thread, type, sizeof(l_service_message));
  msg->svid = svid;
  msg->fd = fd;
  l_send_message(thread, destid, &msg->head);
}

void l_send_bootstrap_message(l_thread* thread, int (*bootstrap)()) {
  l_bootstrap_message* msg = (l_bootstrap_message*)l_create_message(thread, L_MESSAGE_BOOTSTRAP, sizeof(l_bootstrap_message));
  msg->bootstrap = bootstrap;
  l_send_message(thread, L_SERVICE_BOOTSTRAP, &msg->head);
}

