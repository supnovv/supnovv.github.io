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

l_thrkey llg_thread_key;
l_thrkey llg_logger_key;

l_thread* llg_thread_ptr;
l_logger* llg_logger_ptr;

static l_priorq llg_thread_prq;

static llglobalinit() {
  l_thrkey_init(&llg_thread_key);
  l_thrkey_init(&llg_logger_key);
  l_prioriq_init(&llg_thread_prq);
}

typedef struct {
  L_COMMON_BUFHEAD
} l_bufhead;

typedef struct {
  l_squeue queue;
  l_int size;  /* size of queue */
  l_int total; /* total newed elems */
  l_int ttmem; /* total memory size */
  l_int limit; /* memory size limit */
} l_freeq;

static void llfreeqinit(l_freeq* self) {
  *self = (l_freeq){{{0},0}, 0};
  l_squeue_init(&self->queue);
}

static void llfreeqfree(l_freeq* self, void (*elemfree)(void*)) {
  l_smplnode* node = 0;
  while ((node = l_squeue_pop(&self->queue))) {
    if (elemfree) elemfree(node);
    l_raw_free(node);
  }
}

void* l_thread_ensure_bfsize(l_thread* thread, l_smplnode* elem, l_int newsz) {
  l_int bufsz = ((l_bufhead*)elem)->bsize;
  l_freeq* q = thread->frbf;

  if (bufsz >= newsz) {
    return elem;
  }

  if (newsz > l_max_rdwr_size) {
    l_loge_1("large %d", ld(sizeofelem));
    return 0;
  }

  while (bufsz < newsz) {
    if (bufsz <= l_max_rdwr_size / 2) sz *= 2;
    else bufsz = l_max_rdwr_size;
  }

  q->ttmem += bufsz - ((l_bufhead*)elem)->bsize;
  elem = l_raw_realloc(elem, ((l_bufhead*)elem)->bsize, bufsz);
  ((l_bufhead*)elem)->bsize = bufsz;
  return elem;
}

void* l_thread_alloc_buffer(l_thread* thread, l_int sizeofelem) {
  l_smplnode* elem = 0;
  l_freeq* q = thread->frbf;
  void* temp = 0;

  if ((elem = l_squeue_pop(&q->queue))) {
    temp = l_freeq_ensure_size(q, elem, sizeofelem);

    if (!temp) {
      l_squeue_push(&q->queue, elem);
      return 0;
    }

    q->size -= 1;
    return temp;
  }

  q->total += 1;
  q->ttmem += sizeofelem;
  elem = (l_bufhead*)l_raw_malloc(sizeofelem);
  elem->bsize = sizeofelem;
  return elem;
}

void l_freeq_free_buffer(l_thread* thread, l_smplnode* elem, void (*elemfree)(void*)) {
  l_freeq* q = thread->frbf;

  l_squeue_push(&q->queue, elem);
  q->size += 1;

  if (q->size / 4 <= q->total) {
    return;
  }

  while (q->size / 2 > max) {
    if (!(elem = l_squeue_pop(&q->queue))) {
      break;
    }

    q->size -= 1;
    q->total -= 1;
    q->ttmem -= ((l_bufhead*)elem)->bsize;

    if (elemfree) elemfree(elem);
    l_raw_free(elem);
  }
}

typedef struct {
  l_mutex ma;
  l_mutex mb;
  l_condv ca;
  l_logger l;
  l_freeq co;
  l_freeq bf;
} l_thrblock;

static void llallocthreads(int numofthread) {
  l_thread* t = 0;
  while (numofthread-- > 0) {
    t = (l_thread*)l_raw_calloc(sizeof(l_thread));
    t->thrblock = l_raw_malloc(sizeof(l_thrblk));
    t->index = (l_ushort)numofthread;
    l_priorq_push(&llg_thread_prq, &t->node);
  }
}

static void llfreethreads() {
  l_thread* t = 0;
  while ((t = (l_thread*)l_priorq_pop(&llg_thread_prq))) {
    l_raw_free(t->tb);
    l_raw_free(t);
  }
}

static int llthreadless(void* elem, void* elem2) {
  return ((l_thread*)thread)->weight < ((l_thread*)elem2)->weight;
}

void l_thread_init(l_thread* self) {
  l_thrblock* b = self->thrblock;
  self->elock = &b->ma;
  self->mutext = &b->mb;
  self->condv = &b->ca;
  self->log = &b->l;
  self->frco = &b->co;
  self->frbf = &b->bf;

  l_mutex_init(self->elock);
  l_mutex_init(self->mutex);
  l_condv_init(self->condv);

  l_dqueue_init(&self->workrxq);
  l_dqueue_init(&self->workq);
  l_squeue_init(&self->msgq);

  self->L = l_lua_newstate();
  l_logger_init(self->log);
  l_freeq_init(self->frco);
  l_freeq_init(self->frbf);
}

void l_thread_free(l_thread* self) {
  if (self->elock) {
    l_mutex_free(self->elock);
    self->elock = 0;
  }

  if (self->mutex) {
    l_mutex_free(self->mutex);
    self->mutex = 0;
  }

  if (self->condv) {
    l_condv_free(self->condv);
    self->condv = 0;
  }

  if (self->L) {
   l_close_luastate(self->L);
   self->L = 0;
  }

  if (self->log) {
    l_logger_free(self->log);
    self->log = 0;
  }

  if (self->frco) {
    l_freeq_free(self->frco);
    self->frco = 0;
  }

  if (self->frbf) {
    l_freeq_free(self->frbf, 0);
    self->frbf = 0;
  }

  /* TODO: free elems in workrxq/workq/msgq */
}

static void lllockthread(l_thread* self) {
  l_mutex_lock(self->mutex);
}

static void llunlockthread(l_thread* self) {
  l_mutex_unlock(self->mutex);
}

static void* llthreadfunc(void* para) {
  int n = 0;
  l_thread* self = (l_thread*)para;
  l_thrkey_set_data(&llg_thread_key, self);
  n = self->start();
  l_thread_free(self);
  return (void*)(l_intptr)n;
}

int l_thread_start(l_thread* self, int (*start)()) {
  self->start = start;
  return l_raw_create_thread(&self->id, llthreadfunc, self);
}

void l_thread_exit() {
  l_thread_free(l_thread_self());
  l_raw_thread_exit();
}

int l_thread_join(l_thread* self) {
  return l_raw_thread_join(&self->id);
}

/* no protacted, can only be called by master */
static l_thread* ll_get_thread_for_new_service() {
  llthreads* self = ll_ccg_threads();
  l_thread* thread = (l_thread*)ccpriorq_pop(&self->priq);
  thread->weight += 1;
  l_priorq_push(&self->priq, &thread->node);
  return thread;
}

/* no protacted, can only be called by master */
static void ll_thread_finish_a_service(l_thread* thread) {
  llthreads* self = ll_ccg_threads();
  if (thread->weight > 0) {
    thread->weight -= 1;
    l_priorq_remove(&self->priq, &thread->node);
    l_priorq_push(&self->priq, &thread->node);
  }
}

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

typedef struct {
  l_squeue rxq;
  l_squeue freeq;
  l_umedit freesize;
  l_umedit total;
  l_mutex mutex;
} llmessage;

static llmessages* ll_ccg_messages();
static void ll_msg_free_data(l_message* self);

static void llmessages_init(llmessages* self) {
  l_squeue_init(&self->rxq);
  l_squeue_init(&self->freeq);
  l_mutex_init(&self->mutex);
  self->freesize = self->total = 0;
}

static void llmessages_free(llmessages* self) {
  l_message* msg = 0;

  l_mutex_lock(&self->mutex);
  l_squeue_pushqueue(&self->freeq, &self->rxq);
  l_mutex_unlock(&self->mutex);

  while ((msg = (l_message*)ccsqueue_pop(&self->freeq))) {
    ll_msg_free_data(msg);
    l_rawfree(msg);
  }

  l_mutex_free(&self->mutex);
}

static void ll_wakeup_master() {
  l_ionfmgr* ionf = ll_get_ionfmgr();
  l_ionfmgr_wakeup(ionf);
}

static void ll_send_message(l_message* msg) {
  llmessages* self = ll_ccg_messages();

  /* push the message */
  l_mutex_lock(&self->mutex);
  l_squeue_push(&self->rxq, &msg->node);
  l_mutex_unlock(&self->mutex);

  /* wakeup master to handle the message */
  ll_wakeup_master();
}

static l_squeue ll_get_current_msgs() {
  llmessages* self = ll_ccg_messages();
  l_squeue q;

  l_mutex_lock(&self->mutex);
  q = self->rxq;
  l_squeue_init(&self->rxq);
  l_mutex_lock(&self->mutex);

  return q;
}

static l_message* ll_new_msg() {
  llmessages* self = ll_ccg_messages();
  l_message* msg = 0;

  l_mutex_lock(&self->mutex);
  msg = (l_message*)ccsqueue_pop(&self->freeq);
  if (self->freesize > 0) {
    self->freesize -= 1;
  }
  l_mutex_unlock(&self->mutex);

  if (msg == 0) {
    msg = (l_message*)ccrawalloc(sizeof(l_message));
    self->total += 1;
  }
  msg->node.next = msg->extra = 0;
  return msg;
}

static void ll_msg_free_data(l_message* self) {
  if (self->data.ptr && (self->flag & L_MESSAGE_FLAG_FREEPTR)) {
    l_rawfree(self->data.ptr);
  }
  self->extra = self->data.ptr = 0;
  self->flag = self->type = 0;
}

static void ll_free_msgs(l_squeue* msgq) {
  llmessages* self = ll_ccg_messages();
  l_smplnode* elem = 0, * head = 0;
  l_umedit count = 0;

  head = &msgq->head;
  for (elem = head->next; elem != head; elem = elem->next) {
    count += 1;
    ll_msg_free_data((l_message*)elem);
  }

  if (count) {
    l_mutex_lock(&self->mutex);
    l_squeue_pushqueue(&self->freeq, msgq);
    self->freesize += count;
    l_mutex_unlock(&self->mutex);
  }
}

struct ccglobal {
  llthreads threads;
  llservices services;
  llevents events;
  llmessages messages;
  l_ionfmgr ionf;
  l_thread* mainthread;
};

static l_global* ccG;

static llthreads* ll_ccg_threads() {
  return &ccG->threads;
}

static llservices* ll_ccg_services() {
  return &ccG->services;
}

static llevents* ll_ccg_events() {
  return &ccG->events;
}

static llmessages* ll_ccg_messages() {
  return &ccG->messages;
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

l_message* message_create(l_umedit type, l_umedit flag) {
  l_message* msg = ll_new_msg();
  msg->type = type;
  msg->flag = flag;
  return msg;
}

void* message_set_specific(l_message* self, void* data) {
  self->data.ptr = data;
  return data;
}

void* message_set_allocated_specific(l_message* self, int bytes) {
  self->flag |= L_MESSAGE_FLAG_FREEPTR;
  self->data.ptr = l_rawalloc(bytes);
  return self->data.ptr;
}

static void ll_service_send_message(l_service* service, l_umedit destid, l_message* msg) {
  msg->srcid = service->svid;
  msg->dstid = destid;
  ll_send_message(msg);
}

static void ll_service_send_message_sp(l_service* service, l_umedit destid, l_umedit type, void* static_ptr) {
  l_message* msg = message_create(type, 0);
  message_set_specific(msg, static_ptr);
  ll_service_send_message(service, destid, msg);
}

static void ll_service_send_message_um(l_service* service, l_umedit destid, l_umedit type, l_umedit data) {
  l_message* msg = message_create(type, 0);
  msg->data.u32 = data;
  ll_service_send_message(service, destid, msg);
}

static void ll_service_send_message_fd(l_service* service, l_umedit destid, l_umedit type, handle_int fd) {
  l_message* msg = message_create(type, 0);
  msg->data.fd = fd;
  ll_service_send_message(service, destid, msg);
}

void service_send_message(l_state* state, l_umedit destid, l_message* msg) {
  ll_service_send_message(state->srvc, destid, msg);
}

void service_send_message_sp(l_state* state, l_umedit destid, l_umedit type, void* static_ptr) {
  ll_service_send_message_sp(state->srvc, destid, type, static_ptr);
}

void service_send_message_um(l_state* state, l_umedit destid, l_umedit type, l_umedit data) {
  ll_service_send_message_um(state->srvc, destid, type, data);
}

void service_send_message_fd(l_state* state, l_umedit destid, l_umedit type, handle_int fd) {
  ll_service_send_message_fd(state->srvc, destid, type, fd);
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

static void ll_accept_connection(void* ud, l_sockconn* conn) {
  l_service* sv = (l_service*)ud;
  ll_service_send_message_fd(sv, sv->svid, L_MESSAGE_CONNIND, conn->sock);
}

static void l_master_dispatch_event(l_ioevent* event) {
  l_umedit svid = event->udata;
  l_service* sv = 0;
  l_umedit type = 0;

  sv = ll_find_service(svid);
  if (sv == 0) {
    l_loge("service not found");
    return;
  }

  if (event->fd != sv->event->fd) {
    l_loge("fd mismatch");
    return;
  }

  ll_lock_event(sv);
  if (sv->event->flags & L_EVENT_FLAG_LISTEN) {
    if (event->masks & L_EVENT_READ) {
      type = L_MESSAGE_CONNIND;
    }
  } else if (sv->event->flags & L_EVENT_FLAG_CONNECT) {
    if (event->masks & L_EVENT_WRITE) {
      sv->event->flags &= (~((l_ushort)L_EVENT_FLAG_CONNECT));
      type = L_MESSAGE_CONNRSP;
    }
  } else {
    if (sv->event->masks == 0) {
      type = L_MESSAGE_IOEVENT;
    }
    sv->event->masks |= event->masks;
  }
  ll_unlock_event(sv);

  switch (type) {
  case L_MESSAGE_CONNIND:
    l_socket_accept(sv->event->fd, ll_accept_connection, sv);
    break;
  case L_MESSAGE_CONNRSP:
    ll_service_send_message_um(sv, sv->svid, L_MESSAGE_CONNRSP, 0);
    break;
  case L_MESSAGE_IOEVENT:
    ll_service_send_message_um(sv, sv->svid, L_MESSAGE_IOEVENT, 0);
    break;
  }
}

void l_master_start() {
  l_squeue queue;
  l_squeue svmsgs;
  l_squeue freeq;
  l_message* msg = 0;
  l_service* sv = 0;
  l_thread* master = l_thread_getmaster();

  /* read parameters from config file and start worker threads */

  for (; ;) {
    l_ionfmgr_wait(ll_get_ionfmgr(), l_master_dispatch_event);

    /* get all received messages */
    queue = ll_get_current_msgs();

    /* prepare master's messages */
    l_squeue_init(&svmsgs);
    while ((msg = (l_message*)ccsqueue_pop(&queue))) {
      if (msg->dstid == L_SERVICE_MASTERID) {
        l_squeue_push(&master->msgq, &msg->node);
      } else {
        l_squeue_push(&svmsgs, &msg->node);
      }
    }

    /* handle master's messages */
    l_squeue_init(&freeq);
    while ((msg = (l_message*)ccsqueue_pop(&master->msgq))) {
      switch (msg->type) {
      case L_MESSAGE_RUNSERVICE:
        sv = (l_service*)msg->data.ptr;
        if (sv->event) {
          l_ionfmgr_add(ll_get_ionfmgr(), sv->event);
        }
        ll_add_service_to_table(sv);
        break;
      case L_MESSAGE_ADDEVENT:
        if (msg->data.ptr) {
          l_ioevent* event = (l_ioevent*)msg->data.ptr;
          l_ionfmgr_add(ll_get_ionfmgr(), event);
        }
        break;
      case L_MESSAGE_DELEVENT:
        if (msg->data.ptr) {
          l_ioevent* event = (l_ioevent*)msg->data.ptr;
          l_ionfmgr_del(ll_get_ionfmgr(), event);
          /* need free the event after it is deleted */
          ll_free_event(event);
        }
        break;
      case L_MESSAGE_DELSERVICE:
        sv = (l_service*)msg->data.ptr;
        /* service's received msgs (if any) */
        l_squeue_pushqueue(&freeq, &sv->rxmq);
        /* detach and free event if any */
        if (sv->event) {
          l_ionfmgr_del(ll_get_ionfmgr(), sv->event);
          ll_free_event(sv->event);
          sv->event = 0;
        }
        /* remove service out from hash table and insert to free queue */
        ll_service_is_finished(sv);
        /* reset the service */
        sv->belong = 0;
        break;
      default:
        l_loge("unknow message id");
        break;
      }
      l_squeue_push(&freeq, &msg->node);
    }

    /* dispatch service's messages */
    while ((msg = (l_message*)ccsqueue_pop(&svmsgs))) {
      nauty_bool newattach = false;

      /* get message's dest service */
      sv = ll_find_service(msg->dstid);
      if (sv == 0) {
        l_logw("service not found");
        l_squeue_push(&freeq, &msg->node);
        continue;
      }

      if (sv->belong) {
        nauty_bool done = false;

        ll_lock_thread(sv->belong);
        done = (sv->outflag & L_SERVICE_COMPLETED) != 0;
        ll_unlock_thread(sv->belong);

        if (done) {
          /* free current msg and service's received msgs (if any) */
          l_squeue_push(&freeq, &msg->node);
          l_squeue_pushqueue(&freeq, &sv->rxmq);

          /* when service is done, the service is already removed out from thread's service queue.
          here delete service from the hash table and insert into service's free queue */
          ll_service_is_finished(sv);

          /* reset the service */
          sv->belong = 0;
          continue;
        }
      } else {
        /* attach a thread to handle the service */
        sv->belong = ll_get_thread_for_new_service();
        newattach = true;
      }

      /* queue the service to its belong thread */
      ll_lock_thread(sv->belong);
      if (newattach) {
        sv->outflag = sv->flag = 0;
        l_dqueue_push(&sv->belong->workrxq, &sv->node);
      }
      msg->extra = sv;
      l_squeue_push(&sv->rxmq, &msg->node);
      sv->belong->missionassigned = true;
      ll_unlock_thread(sv->belong);
      /* signal the thread to handle */
      l_condv_signal(&sv->belong->condv);
    }

    ll_free_msgs(&freeq);
  }
}

void l_worker_start() {
  l_dqueue doneq;
  l_squeue freemq;
  l_service* service = 0;
  l_linknode* head = 0;
  l_linknode* elem = 0;
  l_message* msg = 0;
  l_thread* thread = l_thread_getself();

  for (; ;) {
    l_mutex_lock(&thread->mutex);
    while (!thread->missionassigned) {
      l_condv_wait(&thread->condv, &thread->mutex);
    }
    /* mission waited, reset false */
    thread->missionassigned = false;
    l_dqueue_pushqueue(&thread->workq, &thread->workrxq);
    l_mutex_unlock(&thread->mutex);

    head = &(thread->workq.head);
    for (elem = head->next; elem != head; elem = elem->next) {
      service = (l_service*)elem;
      if (!ccsqueue_isempty(&service->rxmq)) {
        ll_lock_thread(thread);
        l_squeue_pushqueue(&thread->msgq, &service->rxmq);
        ll_unlock_thread(thread);
      }
    }

    if (ccsqueue_isempty(&thread->msgq)) {
      /* there are no messages received */
      continue;
    }

    l_dqueue_init(&doneq);
    l_squeue_init(&freemq);
    while ((msg = (l_message*)ccsqueue_pop(&thread->msgq))) {
      service = (l_service*)msg->extra;
      if (service->flag & L_SERVICE_COMPLETED) {
        l_squeue_push(&freemq, &msg->node);
        continue;
      }

      if (msg->type == L_MESSAGE_IOEVENT) {
        ll_lock_thread(thread);
        msg->data.u32 = service->event->masks;
        service->event->masks = 0;
        ll_unlock_thread(thread);
        if (msg->data.u32 == 0) {
          l_squeue_push(&freemq, &msg->node);
          continue;
        }
      }

#if 0
      if (service->flag & L_SERVICE_YIELDABLE) {
        if (service->co == 0) {
          service->co = ll_new_coroutine(thread, service);
        }
        if (service->co->L != thread->L) {
          l_loge("wrong lua state");
          l_squeue_push(&freemq, &msg->node);
          continue;
        }
        service->co->srvc = service;
        service->co->msg = msg;
        l_state_resume(service->co);
      } else {

      }
#endif

      /* SERVICE要么保存在hashtable中，要么保存在free队列中，
　　　　　 一个SERVICE只分配给一个线程处理，当某个SERVICE的消息到来
      时, 从消息可以获取SERVICE号，由SERVICE号从hashtable中
      取得service对象，这个service对象会保存到消息extra中，然
      后将消息插入线程的消息队列中等待线程处理，
      service->entry(service, msg);

      /* service run completed */
      if (service->flag & L_SERVICE_COMPLETED) {
        l_dqueue_push(&doneq, l_linknode_remove(&service->node));
        if (service->co) {
          ll_free_coroutine(thread, service->co);
          service->co = 0;
        }
      }

      /* free handled message */
      l_squeue_push(&freemq, &msg->node);
    }

    while ((service = (l_service*)ccdqueue_pop(&doneq))) {
      ll_lock_thread(thread);
      service->outflag |= L_SERVICE_COMPLETED;
      ll_unlock_thread(thread);

      /* currently the finished service is removed out from thread's
      service queue, but the service still pointer to this thread.
      send SVDONE message to master to reset and free this service. */
      ll_service_send_message_sp(service, L_SERVICE_MASTERID, L_MESSAGE_DELSERVICE, service);
    }

    ll_free_msgs(&freemq);
  }
}

static void ll_parse_command_line(int argc, char** argv) {
  (void)argc;
  (void)argv;
}

int startmainthreadcv(int (*start)(), int argc, char** argv) {
  int n = 0;
  l_global G;
  l_thread master;

  /* init global first */
  l_global_init(&G);

  /* init master thread */
  l_thread_init(&master);
  master.id = l_plat_selfthread();

  /* attach master thread */
  l_global_setmaster(&master);

  /* call start func */
  ll_parse_command_line(argc, argv);
  n = start();

  /* clean */
  l_thread_free(&master);
  l_global_free(&G);
  return n;
}

int startmainthread(int (*start)()) {
  return startmainthreadcv(start, 0, 0);
}

