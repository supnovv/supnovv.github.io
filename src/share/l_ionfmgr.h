#ifndef l_ionfmgr_lib_h
#define l_ionfmgr_lib_h
#include "thatcore.h"

#define L_IOEVENT_READ  0x01
#define L_IOEVENT_WRITE 0x02
#define L_IOEVENT_RDWR  0x03
#define L_IOEVENT_PRI   0x04
#define L_IOEVENT_RDH   0x08
#define L_IOEVENT_HUP   0x10
#define L_IOEVENT_ERR   0x20

#define L_IOEVENT_FLAG_ADDED   0x01
#define L_IOEVENT_FLAG_LISTEN  0x02
#define L_IOEVENT_FLAG_CONNECT 0x04

typedef struct l_ioevent {
  l_smplnode node;
  l_handle sock;
  l_umedit svid;
  l_ushort masks;
  l_ushort flags;
  l_service* chained;
} l_ioevent;

typedef struct {
  L_PLAT_IMPL_SIZE(L_IONFMGR_SIZE);
} l_ionfmgr;

l_extern int l_ionfmgr_init(l_ionfmgr* self);
l_extern void l_ionfmgr_free(l_ionfmgr* self);
l_extern int l_ionfmgr_add(l_ionfmgr* self, l_ioevent* event);
l_extern int l_ionfmgr_mod(l_ionfmgr* self, l_ioevent* event);
l_extern int l_ionfmgr_del(l_ionfmgr* self, l_ioevent* event);
l_extern int l_ionfmgr_wait(l_ionfmgr* self, void (*cb)(l_ioevent*));
l_extern int l_ionfmgr_trywait(l_ionfmgr* self, void (*cb)(l_ioevent*));
l_extern int l_ionfmgr_timedwait(l_ionfmgr* self, int ms, void (*cb)(l_ioevent*));
l_extern int l_ionfmgr_wakeup(l_ionfmgr* self);

#endif /* l_ionfmgr_lib_h */

