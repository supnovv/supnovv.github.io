#ifndef l_service_lib_h
#define l_service_lib_h
#include "thatcore.h"
#include "l_message.h"

#define L_SERVICE_MASTERID (0)

typedef struct l_service {
  l_smplnode node; /* only accessed by master */
  l_ioevent* ioev; /* guard by svmtx except ioev->node */
  l_byte msflags; /* only accessed by master */
  l_byte outflags; /* guard by svmtx */
  /* thread own use */
  l_byte flags; /* only accessed by a worker */
  l_umedit svid; /* only set once when init, so can freely access it */
  l_thread* belong; /* only set once when init, so can freely access it */
  l_state* co; /* only accessed by a worker */
  int (*entry)(l_service*, l_message*); /* only accessed by a worker */
} l_service;

l_extern l_service* l_create_service(l_thread* thread, l_int size, int (*entry)(l_service*, l_message*), int samethread);
l_extern void l_start_service(l_service* srvc);
l_extern void l_start_listener_service(l_service* srvc, l_handle sock);
l_extern void l_start_initiator_service(l_service* srvc, l_handle sock);
l_extern void l_start_receiver_service(l_service* srvc, l_handle sock);
l_extern void l_stop_service(l_service* srvc);
l_extern void l_close_socket(l_service* srvc);
l_extern int l_service_resume(l_service* self, int (*func)(l_state*));
l_extern int l_service_yield(l_service* self, int (*kfunc)(l_state*));

#endif /* l_service_lib_h */

