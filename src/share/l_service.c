#include "l_service.h"

#define L_MESSAGE_START_SERVICE 0x01
#define L_MESSAGE_STOP_SERVICE 0x02
#define L_MESSAGE_REMOVE_EVENT 0x03

#define L_SERVICE_RUNNING    0x01
#define L_SERVICE_SAMETHREAD 0x04

extern l_umedit l_master_gen_service_id();

l_service* l_create_service(l_thread* thread, l_int size, int (*entry)(l_service*, l_message*), int insamethread) {
  l_debug_assert(thread == l_thread_self());
  l_service* srvc = l_thread_pop_buffer(thread, size);
  srvc->svid = l_master_gen_service_id();
  srvc->ioev = 0;
  srvc->belong = thread;
  srvc->co = 0;
  srvc->entry = entry;
  srvc->flags = srvc->outflags = 0;
  if (insamethread) {
    srvc->flags |= L_SERVICE_SAMETHREAD;
  } else {
    srvc->flags &= (~L_SERVICE_SAMETHREAD);
  }
}

void l_start_service(l_service* srvc) {
  l_thread* thread = srvc->belong;
  if (!(srvc->flags & L_SERVICE_SAMETHREAD)) {
    srvc->belong = 0;
  }
  l_send_message_up(thread, L_SERVICE_MASTER_ID, L_MESSAGE_START_SERVICE, srvc);
}

static void ll_start_ioevent_service(l_service* srvc, l_handle sock, l_ushort masks, l_ushort flags) {
  l_thread* thread = srvc->belong;
  l_ioevent* ioev = 0;

  if (!l_socket_is_open(sock)) {
    l_logw_1("sock %d", ld(sock));
    return l_start_service(srvc);
  }

  ioev = (l_ioevent*)l_thread_alloc_buffer(thread, sizeof(l_ioevent));
  ioev->sock = sock;
  ioev->svid = srvc->svid;
  ioev->masks = masks;
  ioev->flags = flags;
  ioev->chained = 0;
  srvc->ioev = ioev;
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

void l_stop_service(l_service* srvc) {
  srvc->flags &= (~L_SERVICE_RUNNING);
}

void l_close_socket(l_service* srvc) {
  l_thread* thread = srvc->belong;
  l_mutex* svmtx = thread->svmtx;
  l_ioevent* ioev = 0;
  l_handle sock = -1;

  l_mutex_lock(svmtx);
  ioev = srvc->ioev;
  if (ioev) {
    sock = ioev->fd;
    ioev->fd = -1;
    if (ioev->masks == 0) {
      srvc->ioev = 0;
      l_thread_free_buffer(ioev, 0);
    }
  }
  l_mutex_unlock(svmtx);

  if (sock == -1) return;
  l_send_message_fd(thread, L_SERVICE_MASTER_ID, L_MESSAGE_CLOSE_SOCKET, sock);
}

int l_service_resume(l_service* self, int (*func)(l_state*)) {
  if (self->co == 0) {
    l_thread* thread = self->belong;
    l_debug_assert(thread == l_thread_self());
    self->co = l_thread_alloc_buffer(thread, sizeof(l_state));
    l_state_init(self->co, thread, self, func);
  } else {
    self->co->srvc = self;
    self->co->func = func;
  }
  return l_state_resume(self->co);
}

int l_service_yield(ccservice* self, int (*kfunc)(l_state*)) {
  return l_state_yield(self->co, kfunc);
}

