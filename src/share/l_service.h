#ifndef l_service_lib_h
#define l_service_lib_h
#include "thatcore.h"
#include "l_message.h"

#define L_SERVICE_MASTERID (0)

typedef struct l_service {
  l_smplnode node;
  l_ioevent* ioev;
  l_ushort shareflags;
  /* thread own use */
  l_ushort flags;
  l_umedit svid;
  l_thread* belong; /* set once when init */
  l_state* co;
  int (*entry)(l_service*, l_message*);
} l_service;

l_extern l_service* l_create_service(l_thread* thread, l_int size, int (*entry)(l_service*, l_message*), int samethread);
l_extern void l_start_service(l_service* srvc);
l_extern void l_start_listener_service(l_service* srvc, l_handle sock);
l_extern void l_start_initiator_service(l_service* srvc, l_handle sock);
l_extern void l_start_receiver_service(l_service* srvc, l_handle sock);
l_extern void l_stop_service(l_service* srvc);
l_extern int l_service_resume(l_service* self, int (*func)(l_state*));
l_extern int l_service_yield(ccservice* self, int (*kfunc)(l_state*));

#endif /* l_service_lib_h */

