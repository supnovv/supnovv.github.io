#include <stddef.h>
#include "service.h"
#include "luacapi.h"
#include "ionfmgr.h"
#include "socket.h"

#define L_SERVICE_START_SVID 0x2001
#define L_SERVICE_INITSIZEBITS 8 /* 256 */

#define L_MESSAGE_RUNSERVICE 0x01
#define L_MESSAGE_DELSERVICE 0x02
#define L_MESSAGE_ADDEVENT 0x03
#define L_MESSAGE_DELEVENT 0x04
#define L_MESSAGE_FLAG_FREEPTR 0x01

#define L_SERVICE_COMPLETED  0x01
#define L_SERVICE_NOT_START  0x02
#define L_SERVICE_SAMETHREAD 0x04
#define L_SERVICE_YIELDABLE  0x08
#define L_SERVICE_FREE_DATA  0x10

typedef struct {
  l_backhash table;
  l_dqueue freeq;
  l_umedit freesize;
  l_umedit total;
  l_umedit seed;
  l_mutex mutex;
} llservices;

static llservices* ll_ccg_services();

static void llservices_init(llservices* self, nauty_byte initsizebits) {
  l_backhash_init(&self->table, initsizebits, ll_service_tlink_offset(), (l_umedit(*)(void*))service_get_id);
  l_dqueue_init(&self->freeq);
  l_mutex_init(&self->mutex);
  self->freesize = 0;
  self->seed = L_SERVICE_START_SVID;
}

static void ll_service_free_memory(void* sv) {
  l_service* service = (l_service*)sv;
  if (service->udata && (service->flag & L_SERVICE_FREE_DATA)) {
    l_rawfree(service->udata);
    service->udata = 0;
  }
  l_rawfree(service);
}

static void llservices_free(llservices* self) {
  l_service* sv = 0;
  l_hashtable_foreach(&self->table.a, ll_service_free_memory);
  l_hashtable_foreach(&self->table.b, ll_service_free_memory);

  while ((sv = (l_service*)ccdqueue_pop(&self->freeq))) {
    ll_service_free_memory(sv);
  }

  l_backhash_free(&self->table);
  l_mutex_free(&self->mutex);
}

static l_umedit ll_new_service_id() {
  llservices* self = ll_ccg_services();
  l_umedit svid = 0;

  l_mutex_lock(&self->mutex);
  svid = self->seed++;
  l_mutex_unlock(&self->mutex);

  return svid;
}

static void ll_add_service_to_table(l_service* service) {
  llservices* self = ll_ccg_services();
  l_mutex_lock(&self->mutex);
  l_backhash_add(&self->table, service);
  l_mutex_unlock(&self->mutex);
}

static l_service* ll_find_service(l_umedit svid) {
  llservices* self = ll_ccg_services();
  l_service* service = 0;

  l_mutex_lock(&self->mutex);
  service = (l_service*)ccbackhash_find(&self->table, svid);
  l_mutex_unlock(&self->mutex);

  return service;
}

static void ll_service_is_finished(l_service* service) {
  llservices* self = ll_ccg_services();
  ll_thread_finish_a_service(service->belong);

  l_mutex_lock(&self->mutex);
  l_backhash_del(&self->table, service->svid);
  l_dqueue_push(&self->freeq, &service->node);
  self->freesize += 1;
  l_mutex_unlock(&self->mutex);
}

typedef struct {
  l_squeue freeq;
  l_umedit freesize;
  l_umedit total;
  l_mutex mutex;
} llevents;

static llevents* ll_ccg_events();
static l_ionfmgr* ll_get_ionfmgr();

static void llevents_init(llevents* self) {
  l_squeue_init(&self->freeq);
  l_mutex_init(&self->mutex);
  self->freesize = self->total = 0;
}

static void llevents_free(llevents* self) {
  l_ioevent* event;

  l_mutex_lock(&self->mutex);
  while ((event = (l_ioevent*)ccsqueue_pop(&self->freeq))) {
    l_rawfree(event);
  }
  self->freesize = 0;
  l_mutex_unlock(&self->mutex);

  l_mutex_free(&self->mutex);
}

static void ll_lock_event(l_service* service) {
  l_mutex_lock(&service->belong->elock);
}

static void ll_unlock_event(l_service* service) {
  l_mutex_unlock(&service->belong->elock);
}

l_ioevent* ll_new_event() {
  llevents* self = ll_ccg_events();
  l_ioevent* event = 0;

  l_mutex_lock(&self->mutex);
  event = (l_ioevent*)ccsqueue_pop(&self->freeq);
  if (self->freesize) {
    self->freesize -= 1;
  }
  l_mutex_unlock(&self->mutex);

  if (event == 0) {
    event = l_rawalloc(sizeof(l_ioevent));
    self->total += 1;
  }

  l_zero_l(event, sizeof(l_ioevent));
  event->fd = -1;
  return event;
}

void ll_free_event(l_ioevent* event) {
  llevents* self = ll_ccg_events();
  if (event == 0) return;

  if (event->fd != -1) {
    l_socket_close(event->fd);
    event->fd = -1;
  }

  l_mutex_lock(&self->mutex);
  l_squeue_push(&self->freeq, &event->node);
  self->freesize += 1;
  l_mutex_unlock(&self->mutex);
}

void l_service_detach_event(l_service* self) {
  l_ionfmgr* mgr = ll_get_ionfmgr();
  if (self->event && l_socket_isopen(self->event->fd) && (self->event->flags & L_EVENT_FLAG_ADDED)) {
    self->event->flags &= (~L_EVENT_FLAG_ADDED);
    l_ionfmgr_del(mgr, self->event);
  }
}

void l_service_attach_event(l_service* self, handle_int fd, l_ushort masks, l_ushort flags) {
  l_service_detach_event(self);
  if (!self->event) {
    self->event = ll_new_event();
  }
  self->event->fd = fd;
  self->event->udata = self->svid;
  self->event->masks = (masks | L_EVENT_ERR);
  self->event->flags = flags;
  l_ionfmgr_add(ll_get_ionfmgr(), self->event);
  self->event->flags |= L_EVENT_FLAG_ADDED;
}

static void l_global_init(l_global* self) {
  l_zero_l(self, sizeof(l_global));
  llthreads_init(&self->threads);
  llservices_init(&self->services, L_SERVICE_INITSIZEBITS);
  llevents_init(&self->events);
  llmessages_init(&self->messages);

  l_ionfmgr_init(&self->ionf);
  l_thrkey_init(&self->thrkey);

  ccG = self;
}

static void l_global_setmaster(l_thread* master) {
  ccG->mainthread = master;
  ll_set_thread_data(master);
}

static void l_global_free(l_global* self) {
  llthreads_free(&self->threads);
  llservices_free(&self->services);
  llevents_free(&self->events);
  llmessages_free(&self->messages);

  l_ionfmgr_free(&self->ionf);
  l_thrkey_free(&self->thrkey);
}

static l_ionfmgr* ll_get_ionfmgr() {
  return &(ccG->ionf);
}

static l_state* ll_new_state(l_service* srvc, int (*func)(l_state*)) {
  l_state* co;
  l_thread* thr = srvc->belong;
  if ((co = (l_state*)ccsqueue_pop(&thr->freeco))) {
    thr->freecosz --;
  } else {
    co = (l_state*)ccrawalloc(sizeof(l_state));
    thr->totalco += 1;
  }
  l_state_init(co, thr->L, func, srvc);
  return co;
}

static void ll_free_coroutine(l_thread* thread, l_state* co) {
  l_squeue_push(&thread->freeco, &co->node);
  thread->freecosz += 1;
}

/**
 * service
 */

static void l_service_init(l_service* self) {
  l_zero_l(self, sizeof(l_service));
  l_linknode_init(&self->node);
  l_smplnode_init(&self->tlink);
  l_squeue_init(&self->rxmq);
}

static int llservicetlinkoffset() {
  return offsetof(l_service, tlink);
}

static l_service* ll_new_service(l_umedit svid) {
  llservices* self = ll_ccg_services();
  l_service* service = 0;

  l_mutex_lock(&self->mutex);
  service = (l_service*)ccdqueue_pop(&self->freeq);
  if (self->freesize > 0) {
    self->freesize -= 1;
  }
  l_mutex_unlock(&self->mutex);

  if (service == 0) {
    service = (l_service*)ccrawalloc(sizeof(l_service));
    self->total += 1;
  }

  l_service_init(service);
  if (svid == 0) {
    service->svid = ll_new_service_id();
  } else {
    service->svid = svid;
  }

  return service;
}

ccservice* l_service_new(int (*entry)(ccservice*, l_message*)) {
  l_service* service = ll_new_service(0);
  service->entry = entry;
  service->flag = L_SERVICE_NOT_START;
  return service;
}

ccservice* l_service_newfrom(const void* chunk, int len) {
  (void)chunk;
  (void)len;
  return 0;
}

void l_service_setsamethread(ccservice* self, l_state* state) {
  if (!(self->flag & L_SERVICE_NOT_START)) {
    l_loge("already started");
    return;
  }
  self->belong = state->srvc->belong;
  self->flag |= L_SERVICE_SAMETHREAD;
}

void* l_service_setdata(ccservice* self, void* udata) {
  self->udata = udata;
  return udata;
}

void* l_service_allocdata(ccservice* self, int bytes) {
  self->udata = l_rawalloc(bytes);
  self->flag |= L_SERVICE_FREE_DATA;
  return self->udata;
}

void ll_set_event(l_ioevent* event, handle_int fd, l_umedit udata, l_ushort masks, l_ushort flags) {
  event-> fd = fd;
  event->udata = udata;
  event->masks = (masks | L_EVENT_ERR);
  event->flags = flags;
}

void l_service_setevent(l_service* self, l_handle_int fd, l_ushort masks, l_ushort flags) {
  if (!(self->flag & L_SERVICE_NOT_START)) return;
  ll_free_event(self->event);
  self->event = ll_new_event();
  ll_set_event(self->event, fd, self->svid, masks, flags);
}

void service_set_listen(l_service* service, handle_int fd, l_ushort masks, l_ushort flags) {
  if (service->flag & L_SERVICE_NOT_START) {
    ll_free_event(service->event);
    service->event = ll_new_event();
    ll_set_event(service->event, fd, service->svid, masks, flags);
  }
}

void service_listen_event(l_state* state, handle_int fd, l_ushort masks, l_ushort flags) {
  l_service* service = state->srvc;
  l_ioevent* old = 0;

  ll_lock_event(service);
  old = service->event;
  service->event = ll_new_event();
  ll_set_event(service->event, fd, service->svid, masks, flags);
  ll_unlock_event(service);

  if (old) {
    /* need free this old event after delete operation */
    ll_service_send_message_sp(service, L_SERVICE_MASTERID, L_MESSAGE_DELEVENT, old);
  }
  ll_service_send_message_sp(service, L_SERVICE_MASTERID, L_MESSAGE_ADDEVENT, service->event);
}

void service_remove_listen(l_state* state) {
  l_service* service = state->srvc;
  l_ioevent* old = 0;

  ll_lock_event(service);
  old = service->event;
  service->event = 0;
  ll_unlock_event(service);

  if (old) {
    /* need free this old event after delete operation */
    ll_service_send_message_sp(service, L_SERVICE_MASTERID, L_MESSAGE_DELEVENT, old);
  }
}

void l_service_start(l_service* self) {
  if (!(self->flag & L_SERVICE_NOT_START)) return;
  ll_service_send_message_sp(self, L_SERVICE_MASTERID, L_MESSAGE_RUNSERVICE, self);
  self->flag &= (~L_SERVICE_NOT_START);
}

void service_start_run(l_service* service) {
  if (service->flag & L_SERVICE_NOT_START) {
    ll_service_send_message_sp(service, L_SERVICE_MASTERID, L_MESSAGE_RUNSERVICE, service);
    service->flag &= (~L_SERVICE_NOT_START);
  }
}

void l_service_stop(ccservice* self) {
  /* service->flag is accessed by thread itself */
  self->flag |= L_SERVICE_COMPLETED;
}

int l_service_resume(ccservice* self, int (*func)(l_state*)) {
  if (self->co == 0) {
    self->co = ll_new_state(self, func);
  }
  self->co->srvc = self;
  self->co->func = func;
  return l_state_resume(self->co);
}

int l_service_yield(ccservice* self, int (*kfunc)(l_state*)) {
  return l_state_yield(self->co, kfunc);
}

ccthread* l_service_belong(ccservice* self) {
  return self->belong;
}


