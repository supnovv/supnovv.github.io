#ifndef l_service_lib_h
#define l_service_lib_h
#include "thatcore.h"
#include "l_message.h"
#include "l_ionfmgr.h"
#include "l_state.h"

#define L_SERVICE_MASTER_ID (0) /* worker's default svid is its index (16-bit) */
#define L_SERVICE_BOOTSTRAP (0xffff+1)

typedef struct l_service {
  L_COMMON_BUFHEAD
  l_ioevent* ioev; /* guard by svmtx */
  l_ioevent event; /* guard by svmtx */
  l_byte stop_rx_msg; /* service is closing, guard by svmtx */
  l_byte mflgs; /* only accessed by master */
  /* thread own use */
  l_byte wflgs; /* only accessed by a worker */
  l_umedit svid; /* only set once when init, so can freely access it */
  l_thread* thread; /* only set once when init, so can freely access it */
  l_state* co; /* only accessed by a worker */
  int (*entry)(l_service*, l_message*); /* only accessed by a worker */
} l_service;

l_extern l_service* l_create_service(l_thread* thread, l_int size, int (*entry)(l_service*, l_message*), int samethread);
l_extern void l_start_service(l_service* srvc);
l_extern void l_start_listener_service(l_service* srvc, l_handle sock);
l_extern void l_start_initiator_service(l_service* srvc, l_handle sock);
l_extern void l_start_receiver_service(l_service* srvc, l_handle sock);
l_extern void l_close_service(l_service* srvc);
l_extern void l_close_event(l_service* srvc);
l_extern int l_service_resume(l_service* self, int (*func)(l_state*));
l_extern int l_service_yield(l_service* self, int (*kfunc)(l_state*));

#endif /* l_service_lib_h */

