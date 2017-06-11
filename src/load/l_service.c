#include <stddef.h>
#include "service.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"

#define LL_SERVICE_START_SVID 0x2001
#define LL_SERVICE_INITSIZEBITS 8 /* 256 */

#define LL_MESSAGE_RUNSERVICE 0x01
#define LL_MESSAGE_DELSERVICE 0x02
#define LL_MESSAGE_ADDEVENT 0x03
#define LL_MESSAGE_DELEVENT 0x04
#define LL_MESSAGE_FLAG_FREEPTR 0x01

#define LL_SERVICE_COMPLETED  0x01
#define LL_SERVICE_NOT_START  0x02
#define LL_SERVICE_SAMETHREAD 0x04
#define LL_SERVICE_YIELDABLE  0x08
#define LL_SERVICE_FREE_DATA  0x10

struct ccservice {
  /* shared with master */
  struct cclinknode node;
  struct ccsmplnode tlink;
  struct ccsqueue rxmq;
  struct ccioevent* event;
  ushort_int outflag;
  /* thread own use */
  ushort_int flag;
  umedit_int svid;
  struct ccthread* belong; /* set once when init */
  struct ccstate* co;
  int (*entry)(struct ccservice*, struct ccmessage*);
  void* udata;
};

static void ccservice_init(struct ccservice* self) {
  cczeron(self, sizeof(struct ccservice));
  cclinknode_init(&self->node);
  ccsmplnode_init(&self->tlink);
  ccsqueue_init(&self->rxmq);
}

static int ll_service_tlink_offset() {
  return offsetof(struct ccservice, tlink);
}

struct ccstate* ccservice_getstate(struct ccservice* self) {
  return self->co;
}

umedit_int service_get_id(struct ccservice* service) {
  return service->svid;
}

void* ccservice_getdata(ccservice* self) {
  return self->udata;
}

struct ccmessage* service_get_message(struct ccstate* state) {
  return state->msg;
}

cchandle_int ccservice_eventfd(ccservice* self) {
  return self->event->fd;
}

struct ccthread {
  /* access by master only */
  struct cclinknode node;
  umedit_int weight;
  /* shared with master */
  nauty_bool missionassigned;
  struct ccdqueue workrxq;
  struct ccmutex elock;
  struct ccmutex mutex;
  struct cccondv condv;
  /* free access */
  umedit_int index;
  struct ccthrid id;
  /* thread own use */
  struct lua_State* L;
  int (*start)();
  struct ccdqueue workq;
  struct ccsqueue msgq;
  struct ccsqueue freeco;
  umedit_int freecosz;
  umedit_int totalco;
  struct ccstring defstr;
  /* buffer queue */
  struct ccsqueue freebufq;
  ccnauty_int freememsize;
  ccnauty_int maxfreemem;
};

#define LL_THREAD_MAX_MEMORY (1024 * 1024 * 2) /* 2MB */

static void ll_thread_initbufferq(struct ccthread* thread) {
  ccsqueue_init(&thread->freebufq);
  thread->freememsize = 0;
  thread->maxfreemem = LL_THREAD_MAX_MEMORY;
}

static void ll_thread_freebufferq(struct ccthread* thread) {
  struct ccbuffer* p = 0;
  while ((p = (struct ccbuffer*)ccsqueue_pop(&thread->freebufq))) {
    ccrawfree(p);
  }
}

static struct ccbuffer* ll_thread_getfreebuffer(struct ccthread* thread) {
  struct ccbuffer* p = 0;
  if ((p = (struct ccbuffer*)ccsqueue_pop(&thread->freebufq))) {
    if (thread->freememsize > p->capacity) {
      thread->freememsize -= p->capacity;
    }
  }
  return p;
}

void ccbuffer_ensurecapacity(struct ccbuffer** self, ccnauty_int size) {
  ccnauty_int newcap = (*self)->capacity;
  if (newcap >= size) return;
  while ((newcap *= 2) < size) {
    if (newcap > LL_THREAD_MAX_MEMORY) {
      ccloge("buffer too large");
      return;
    }
  }
  *self = (struct ccbuffer*)ccrawrelloc(*self, sizeof(struct ccbuffer) + (*self)->capacity, sizeof(struct ccbuffer) + newcap);
  (*self)->capacity = newcap;
}

void ccbuffer_ensuresizeremain(struct ccbuffer** self, ccnauty_int remainsize) {
  ccbuffer_ensurecapacity(self, (*self)->size + remainsize);
}

#define LL_BUFFER_INIT_SIZE (64)

struct ccbuffer* ccnewbuffer(struct ccthread* thread, umedit_int maxlimit) {
  struct ccbuffer* p = 0;
  if ((p = ll_thread_getfreebuffer(thread))) {
    ccbuffer_ensurecapacity(&p, LL_BUFFER_INIT_SIZE);
  } else {
    p = (struct ccbuffer*)ccrawalloc(sizeof(struct ccbuffer) + LL_BUFFER_INIT_SIZE);
    p->capacity = LL_BUFFER_INIT_SIZE;
  }
  p->maxlimit = maxlimit;
  p->size = 0;
  return p;
}

void ccfreebuffer(struct ccthread* thread, struct ccbuffer* p) {
  ccsqueue_push(&thread->freebufq, &p->node);
  thread->freememsize += p->capacity;
  while (thread->freememsize > thread->maxfreemem) {
    if (!(p = ll_thread_getfreebuffer(thread))) {
      break;
    }
    ccrawfree(p);
  }
}

static nauty_bool ll_thread_less(void* elem, void* elem2) {
  struct ccthread* thread = (struct ccthread*)elem;
  struct ccthread* thread2 = (struct ccthread*)elem2;
  return thread->weight < thread2->weight;
}

static void ll_lock_thread(struct ccthread* self) {
  ccmutex_lock(&self->mutex);
}

static void ll_unlock_thread(struct ccthread* self) {
  ccmutex_unlock(&self->mutex);
}

static umedit_int ll_new_thread_index();
static void ll_free_msgs(struct ccsqueue* msgq);

void ccthread_init(struct ccthread* self) {
  cczeron(self, sizeof(struct ccthread));
  cclinknode_init(&self->node);
  ccdqueue_init(&self->workrxq);
  ccdqueue_init(&self->workq);
  ccsqueue_init(&self->msgq);
  ccsqueue_init(&self->freeco);
  self->freecosz = self->totalco = 0;

  ccmutex_init(&self->elock);
  ccmutex_init(&self->mutex);
  cccondv_init(&self->condv);

  self->L = cclua_newstate();
  self->defstr = ccemptystr();
  self->index = ll_new_thread_index();
  ll_thread_initbufferq(self);
}

void ccthread_free(struct ccthread* self) {
  struct ccstate* co = 0;

  self->start = 0;
  if (self->L) {
   cclua_close(self->L);
   self->L = 0;
  }

  /* free all un-handled msgs */
  ll_free_msgs(&self->msgq);

  /* un-complete services are all in the hash table,
  it is no need to free them here */

  /* free all coroutines */
  while ((co = (struct ccstate*)ccsqueue_pop(&self->freeco))) {
    ccstate_free(co);
    ccrawfree(co);
  }

  ccmutex_free(&self->elock);
  ccmutex_free(&self->mutex);
  cccondv_free(&self->condv);
  ccstring_free(&self->defstr);
  ll_thread_freebufferq(self);
}


/**
 * threads container
 */

struct llthreads {
  struct ccpriorq priq;
  umedit_int seed;
};

static struct llthreads* ll_ccg_threads();

static void llthreads_init(struct llthreads* self) {
  ccpriorq_init(&self->priq, ll_thread_less);
  self->seed = 0;
}

static void llthreads_free(struct llthreads* self) {
  (void)self;
}

/* no protacted, can only be called by master */
static umedit_int ll_new_thread_index() {
  struct llthreads* self = ll_ccg_threads();
  return self->seed++;
}

/* no protacted, can only be called by master */
static struct ccthread* ll_get_thread_for_new_service() {
  struct llthreads* self = ll_ccg_threads();
  struct ccthread* thread = (struct ccthread*)ccpriorq_pop(&self->priq);
  thread->weight += 1;
  ccpriorq_push(&self->priq, &thread->node);
  return thread;
}

/* no protacted, can only be called by master */
static void ll_thread_finish_a_service(struct ccthread* thread) {
  struct llthreads* self = ll_ccg_threads();
  if (thread->weight > 0) {
    thread->weight -= 1;
    ccpriorq_remove(&self->priq, &thread->node);
    ccpriorq_push(&self->priq, &thread->node);
  }
}


/**
 * services container
 */

struct llservices {
  struct ccbackhash table;
  struct ccdqueue freeq;
  umedit_int freesize;
  umedit_int total;
  umedit_int seed;
  struct ccmutex mutex;
};

static struct llservices* ll_ccg_services();

static void llservices_init(struct llservices* self, nauty_byte initsizebits) {
  ccbackhash_init(&self->table, initsizebits, ll_service_tlink_offset(), (umedit_int(*)(void*))service_get_id);
  ccdqueue_init(&self->freeq);
  ccmutex_init(&self->mutex);
  self->freesize = 0;
  self->seed = LL_SERVICE_START_SVID;
}

static void ll_service_free_memory(void* sv) {
  struct ccservice* service = (struct ccservice*)sv;
  if (service->udata && (service->flag & LL_SERVICE_FREE_DATA)) {
    ccrawfree(service->udata);
    service->udata = 0;
  }
  ccrawfree(service);
}

static void llservices_free(struct llservices* self) {
  struct ccservice* sv = 0;
  cchashtable_foreach(&self->table.a, ll_service_free_memory);
  cchashtable_foreach(&self->table.b, ll_service_free_memory);

  while ((sv = (struct ccservice*)ccdqueue_pop(&self->freeq))) {
    ll_service_free_memory(sv);
  }

  ccbackhash_free(&self->table);
  ccmutex_free(&self->mutex);
}

static umedit_int ll_new_service_id() {
  struct llservices* self = ll_ccg_services();
  umedit_int svid = 0;

  ccmutex_lock(&self->mutex);
  svid = self->seed++;
  ccmutex_unlock(&self->mutex);

  return svid;
}

static void ll_add_service_to_table(struct ccservice* service) {
  struct llservices* self = ll_ccg_services();
  ccmutex_lock(&self->mutex);
  ccbackhash_add(&self->table, service);
  ccmutex_unlock(&self->mutex);
}

static struct ccservice* ll_find_service(umedit_int svid) {
  struct llservices* self = ll_ccg_services();
  struct ccservice* service = 0;

  ccmutex_lock(&self->mutex);
  service = (struct ccservice*)ccbackhash_find(&self->table, svid);
  ccmutex_unlock(&self->mutex);

  return service;
}

static void ll_service_is_finished(struct ccservice* service) {
  struct llservices* self = ll_ccg_services();
  ll_thread_finish_a_service(service->belong);

  ccmutex_lock(&self->mutex);
  ccbackhash_del(&self->table, service->svid);
  ccdqueue_push(&self->freeq, &service->node);
  self->freesize += 1;
  ccmutex_unlock(&self->mutex);
}


/**
 * events container
 */

struct llevents {
  struct ccsqueue freeq;
  umedit_int freesize;
  umedit_int total;
  struct ccmutex mutex;
};

static struct llevents* ll_ccg_events();
static struct ccionfmgr* ll_get_ionfmgr();

static void llevents_init(struct llevents* self) {
  ccsqueue_init(&self->freeq);
  ccmutex_init(&self->mutex);
  self->freesize = self->total = 0;
}

static void llevents_free(struct llevents* self) {
  struct ccioevent* event;

  ccmutex_lock(&self->mutex);
  while ((event = (struct ccioevent*)ccsqueue_pop(&self->freeq))) {
    ccrawfree(event);
  }
  self->freesize = 0;
  ccmutex_unlock(&self->mutex);

  ccmutex_free(&self->mutex);
}

static void ll_lock_event(struct ccservice* service) {
  ccmutex_lock(&service->belong->elock);
}

static void ll_unlock_event(struct ccservice* service) {
  ccmutex_unlock(&service->belong->elock);
}

struct ccioevent* ll_new_event() {
  struct llevents* self = ll_ccg_events();
  struct ccioevent* event = 0;

  ccmutex_lock(&self->mutex);
  event = (struct ccioevent*)ccsqueue_pop(&self->freeq);
  if (self->freesize) {
    self->freesize -= 1;
  }
  ccmutex_unlock(&self->mutex);

  if (event == 0) {
    event = ccrawalloc(sizeof(struct ccioevent));
    self->total += 1;
  }

  cczeron(event, sizeof(struct ccioevent));
  event->fd = -1;
  return event;
}

void ll_free_event(struct ccioevent* event) {
  struct llevents* self = ll_ccg_events();
  if (event == 0) return;

  if (event->fd != -1) {
    ccsocket_close(event->fd);
    event->fd = -1;
  }

  ccmutex_lock(&self->mutex);
  ccsqueue_push(&self->freeq, &event->node);
  self->freesize += 1;
  ccmutex_unlock(&self->mutex);
}

void ccservice_detach_event(struct ccservice* self) {
  struct ccionfmgr* mgr = ll_get_ionfmgr();
  if (self->event && ccsocket_isopen(self->event->fd) && (self->event->flags & CCM_EVENT_FLAG_ADDED)) {
    self->event->flags &= (~CCM_EVENT_FLAG_ADDED);
    ccionfmgr_del(mgr, self->event);
  }
}

void ccservice_attach_event(struct ccservice* self, handle_int fd, ushort_int masks, ushort_int flags) {
  ccservice_detach_event(self);
  if (!self->event) {
    self->event = ll_new_event();
  }
  self->event->fd = fd;
  self->event->udata = self->svid;
  self->event->masks = (masks | CCM_EVENT_ERR);
  self->event->flags = flags;
  ccionfmgr_add(ll_get_ionfmgr(), self->event);
  self->event->flags |= CCM_EVENT_FLAG_ADDED;
}

/**
 * messages container
 */

struct llmessages {
  struct ccsqueue rxq;
  struct ccsqueue freeq;
  umedit_int freesize;
  umedit_int total;
  struct ccmutex mutex;
};

static struct llmessages* ll_ccg_messages();
static void ll_msg_free_data(struct ccmessage* self);

static void llmessages_init(struct llmessages* self) {
  ccsqueue_init(&self->rxq);
  ccsqueue_init(&self->freeq);
  ccmutex_init(&self->mutex);
  self->freesize = self->total = 0;
}

static void llmessages_free(struct llmessages* self) {
  struct ccmessage* msg = 0;

  ccmutex_lock(&self->mutex);
  ccsqueue_pushqueue(&self->freeq, &self->rxq);
  ccmutex_unlock(&self->mutex);

  while ((msg = (struct ccmessage*)ccsqueue_pop(&self->freeq))) {
    ll_msg_free_data(msg);
    ccrawfree(msg);
  }

  ccmutex_free(&self->mutex);
}

static void ll_wakeup_master() {
  struct ccionfmgr* ionf = ll_get_ionfmgr();
  ccionfmgr_wakeup(ionf);
}

static void ll_send_message(struct ccmessage* msg) {
  struct llmessages* self = ll_ccg_messages();

  /* push the message */
  ccmutex_lock(&self->mutex);
  ccsqueue_push(&self->rxq, &msg->node);
  ccmutex_unlock(&self->mutex);

  /* wakeup master to handle the message */
  ll_wakeup_master();
}

static struct ccsqueue ll_get_current_msgs() {
  struct llmessages* self = ll_ccg_messages();
  struct ccsqueue q;

  ccmutex_lock(&self->mutex);
  q = self->rxq;
  ccsqueue_init(&self->rxq);
  ccmutex_lock(&self->mutex);

  return q;
}

static struct ccmessage* ll_new_msg() {
  struct llmessages* self = ll_ccg_messages();
  struct ccmessage* msg = 0;

  ccmutex_lock(&self->mutex);
  msg = (struct ccmessage*)ccsqueue_pop(&self->freeq);
  if (self->freesize > 0) {
    self->freesize -= 1;
  }
  ccmutex_unlock(&self->mutex);

  if (msg == 0) {
    msg = (struct ccmessage*)ccrawalloc(sizeof(struct ccmessage));
    self->total += 1;
  }
  msg->node.next = msg->extra = 0;
  return msg;
}

static void ll_msg_free_data(struct ccmessage* self) {
  if (self->data.ptr && (self->flag & LL_MESSAGE_FLAG_FREEPTR)) {
    ccrawfree(self->data.ptr);
  }
  self->extra = self->data.ptr = 0;
  self->flag = self->type = 0;
}

static void ll_free_msgs(struct ccsqueue* msgq) {
  struct llmessages* self = ll_ccg_messages();
  struct ccsmplnode* elem = 0, * head = 0;
  umedit_int count = 0;

  head = &msgq->head;
  for (elem = head->next; elem != head; elem = elem->next) {
    count += 1;
    ll_msg_free_data((struct ccmessage*)elem);
  }

  if (count) {
    ccmutex_lock(&self->mutex);
    ccsqueue_pushqueue(&self->freeq, msgq);
    self->freesize += count;
    ccmutex_unlock(&self->mutex);
  }
}


/**
 * global
 */

struct ccglobal {
  struct llthreads threads;
  struct llservices services;
  struct llevents events;
  struct llmessages messages;
  struct ccionfmgr ionf;
  struct ccthrkey thrkey;
  struct ccthread* mainthread;
};

static struct ccglobal* ccG;

static struct llthreads* ll_ccg_threads() {
  return &ccG->threads;
}

static struct llservices* ll_ccg_services() {
  return &ccG->services;
}

static struct llevents* ll_ccg_events() {
  return &ccG->events;
}

static struct llmessages* ll_ccg_messages() {
  return &ccG->messages;
}

static nauty_bool ll_set_thread_data(struct ccthread* thread) {
  return ccthrkey_setdata(&ccG->thrkey, thread);
}

static void ccglobal_init(struct ccglobal* self) {
  cczeron(self, sizeof(struct ccglobal));
  llthreads_init(&self->threads);
  llservices_init(&self->services, LL_SERVICE_INITSIZEBITS);
  llevents_init(&self->events);
  llmessages_init(&self->messages);

  ccionfmgr_init(&self->ionf);
  ccthrkey_init(&self->thrkey);

  ccG = self;
}

static void ccglobal_setmaster(struct ccthread* master) {
  ccG->mainthread = master;
  ll_set_thread_data(master);
}

static void ccglobal_free(struct ccglobal* self) {
  llthreads_free(&self->threads);
  llservices_free(&self->services);
  llevents_free(&self->events);
  llmessages_free(&self->messages);

  ccionfmgr_free(&self->ionf);
  ccthrkey_free(&self->thrkey);
}

static struct ccionfmgr* ll_get_ionfmgr() {
  return &(ccG->ionf);
}


/**
 * thread
 */

static struct ccstate* ll_new_state(struct ccservice* srvc, int (*func)(struct ccstate*)) {
  struct ccstate* co;
  struct ccthread* thr = srvc->belong;
  if ((co = (struct ccstate*)ccsqueue_pop(&thr->freeco))) {
    thr->freecosz --;
  } else {
    co = (struct ccstate*)ccrawalloc(sizeof(struct ccstate));
    thr->totalco += 1;
  }
  ccstate_init(co, thr->L, func, srvc);
  return co;
}

static void ll_free_coroutine(struct ccthread* thread, struct ccstate* co) {
  ccsqueue_push(&thread->freeco, &co->node);
  thread->freecosz += 1;
}

struct ccthread* ccthread_getself() {
  return (struct ccthread*)ccthrkey_getdata(&ccG->thrkey);
}

struct ccthread* ccthread_getmaster() {
  return ccG->mainthread;
}

struct ccstring* ccthread_getdefstr() {
  return &(ccthread_getself()->defstr);
}

void ccthread_sleep(uright_int us) {
  ccplat_threadsleep(us);
}

void ccthread_exit() {
  ccthread_free(ccthread_getself());
  ccplat_threadexit();
}

int ccthread_join(struct ccthread* self) {
  return ccplat_threadjoin(&self->id);
}

static void* ll_thread_func(void* para) {
  struct ccthread* self = (struct ccthread*)para;
  int n = 0;
  ll_set_thread_data(self);
  n = self->start();
  ccthread_free(self);
  return (void*)(signed_ptr)n;
}

nauty_bool ccthread_start(struct ccthread* self, int (*start)()) {
  self->start = start;
  return ccplat_createthread(&self->id, ll_thread_func, self);
}


/**
 * service
 */

static struct ccservice* ll_new_service(umedit_int svid) {
  struct llservices* self = ll_ccg_services();
  struct ccservice* service = 0;

  ccmutex_lock(&self->mutex);
  service = (struct ccservice*)ccdqueue_pop(&self->freeq);
  if (self->freesize > 0) {
    self->freesize -= 1;
  }
  ccmutex_unlock(&self->mutex);

  if (service == 0) {
    service = (struct ccservice*)ccrawalloc(sizeof(struct ccservice));
    self->total += 1;
  }

  ccservice_init(service);
  if (svid == 0) {
    service->svid = ll_new_service_id();
  } else {
    service->svid = svid;
  }

  return service;
}

struct ccmessage* message_create(umedit_int type, umedit_int flag) {
  struct ccmessage* msg = ll_new_msg();
  msg->type = type;
  msg->flag = flag;
  return msg;
}

void* message_set_specific(struct ccmessage* self, void* data) {
  self->data.ptr = data;
  return data;
}

void* message_set_allocated_specific(struct ccmessage* self, int bytes) {
  self->flag |= LL_MESSAGE_FLAG_FREEPTR;
  self->data.ptr = ccrawalloc(bytes);
  return self->data.ptr;
}

static void ll_service_send_message(struct ccservice* service, umedit_int destid, struct ccmessage* msg) {
  msg->srcid = service->svid;
  msg->dstid = destid;
  ll_send_message(msg);
}

static void ll_service_send_message_sp(struct ccservice* service, umedit_int destid, umedit_int type, void* static_ptr) {
  struct ccmessage* msg = message_create(type, 0);
  message_set_specific(msg, static_ptr);
  ll_service_send_message(service, destid, msg);
}

static void ll_service_send_message_um(struct ccservice* service, umedit_int destid, umedit_int type, umedit_int data) {
  struct ccmessage* msg = message_create(type, 0);
  msg->data.u32 = data;
  ll_service_send_message(service, destid, msg);
}

static void ll_service_send_message_fd(struct ccservice* service, umedit_int destid, umedit_int type, handle_int fd) {
  struct ccmessage* msg = message_create(type, 0);
  msg->data.fd = fd;
  ll_service_send_message(service, destid, msg);
}

void service_send_message(struct ccstate* state, umedit_int destid, struct ccmessage* msg) {
  ll_service_send_message(state->srvc, destid, msg);
}

void service_send_message_sp(struct ccstate* state, umedit_int destid, umedit_int type, void* static_ptr) {
  ll_service_send_message_sp(state->srvc, destid, type, static_ptr);
}

void service_send_message_um(struct ccstate* state, umedit_int destid, umedit_int type, umedit_int data) {
  ll_service_send_message_um(state->srvc, destid, type, data);
}

void service_send_message_fd(struct ccstate* state, umedit_int destid, umedit_int type, handle_int fd) {
  ll_service_send_message_fd(state->srvc, destid, type, fd);
}

ccservice* ccservice_new(int (*entry)(ccservice*, ccmessage*)) {
  ccservice* service = ll_new_service(0);
  service->entry = entry;
  service->flag = LL_SERVICE_NOT_START;
  return service;
}

ccservice* ccservice_newfrom(const void* chunk, int len) {
  (void)chunk;
  (void)len;
  return 0;
}

void ccservice_setsamethread(ccservice* self, ccstate* state) {
  if (!(self->flag & LL_SERVICE_NOT_START)) {
    ccloge("already started");
    return;
  }
  self->belong = state->srvc->belong;
  self->flag |= LL_SERVICE_SAMETHREAD;
}

void* ccservice_setdata(ccservice* self, void* udata) {
  self->udata = udata;
  return udata;
}

void* ccservice_allocdata(ccservice* self, int bytes) {
  self->udata = ccrawalloc(bytes);
  self->flag |= LL_SERVICE_FREE_DATA;
  return self->udata;
}

void ll_set_event(struct ccioevent* event, handle_int fd, umedit_int udata, ushort_int masks, ushort_int flags) {
  event-> fd = fd;
  event->udata = udata;
  event->masks = (masks | CCM_EVENT_ERR);
  event->flags = flags;
}

void ccservice_setevent(struct ccservice* self, cchandle_int fd, ushort_int masks, ushort_int flags) {
  if (!(self->flag & LL_SERVICE_NOT_START)) return;
  ll_free_event(self->event);
  self->event = ll_new_event();
  ll_set_event(self->event, fd, self->svid, masks, flags);
}

void service_set_listen(struct ccservice* service, handle_int fd, ushort_int masks, ushort_int flags) {
  if (service->flag & LL_SERVICE_NOT_START) {
    ll_free_event(service->event);
    service->event = ll_new_event();
    ll_set_event(service->event, fd, service->svid, masks, flags);
  }
}

void service_listen_event(struct ccstate* state, handle_int fd, ushort_int masks, ushort_int flags) {
  struct ccservice* service = state->srvc;
  struct ccioevent* old = 0;

  ll_lock_event(service);
  old = service->event;
  service->event = ll_new_event();
  ll_set_event(service->event, fd, service->svid, masks, flags);
  ll_unlock_event(service);

  if (old) {
    /* need free this old event after delete operation */
    ll_service_send_message_sp(service, CCM_SERVICE_MASTERID, LL_MESSAGE_DELEVENT, old);
  }
  ll_service_send_message_sp(service, CCM_SERVICE_MASTERID, LL_MESSAGE_ADDEVENT, service->event);
}

void service_remove_listen(struct ccstate* state) {
  struct ccservice* service = state->srvc;
  struct ccioevent* old = 0;

  ll_lock_event(service);
  old = service->event;
  service->event = 0;
  ll_unlock_event(service);

  if (old) {
    /* need free this old event after delete operation */
    ll_service_send_message_sp(service, CCM_SERVICE_MASTERID, LL_MESSAGE_DELEVENT, old);
  }
}

void ccservice_start(struct ccservice* self) {
  if (!(self->flag & LL_SERVICE_NOT_START)) return;
  ll_service_send_message_sp(self, CCM_SERVICE_MASTERID, LL_MESSAGE_RUNSERVICE, self);
  self->flag &= (~LL_SERVICE_NOT_START);
}

void service_start_run(struct ccservice* service) {
  if (service->flag & LL_SERVICE_NOT_START) {
    ll_service_send_message_sp(service, CCM_SERVICE_MASTERID, LL_MESSAGE_RUNSERVICE, service);
    service->flag &= (~LL_SERVICE_NOT_START);
  }
}

void ccservice_stop(ccservice* self) {
  /* service->flag is accessed by thread itself */
  self->flag |= LL_SERVICE_COMPLETED;
}

int ccservice_resume(ccservice* self, int (*func)(struct ccstate*)) {
  if (self->co == 0) {
    self->co = ll_new_state(self, func);
  }
  self->co->srvc = self;
  self->co->func = func;
  return ccstate_resume(self->co);
}

int ccservice_yield(ccservice* self, int (*kfunc)(struct ccstate*)) {
  return ccstate_yield(self->co, kfunc);
}

ccthread* ccservice_belong(ccservice* self) {
  return self->belong;
}

static void ll_accept_connection(void* ud, struct ccsockconn* conn) {
  struct ccservice* sv = (struct ccservice*)ud;
  ll_service_send_message_fd(sv, sv->svid, CCM_MESSAGE_CONNIND, conn->sock);
}

static void ccmaster_dispatch_event(struct ccioevent* event) {
  umedit_int svid = event->udata;
  struct ccservice* sv = 0;
  umedit_int type = 0;

  sv = ll_find_service(svid);
  if (sv == 0) {
    ccloge("service not found");
    return;
  }

  if (event->fd != sv->event->fd) {
    ccloge("fd mismatch");
    return;
  }

  ll_lock_event(sv);
  if (sv->event->flags & CCM_EVENT_FLAG_LISTEN) {
    if (event->masks & CCM_EVENT_READ) {
      type = CCM_MESSAGE_CONNIND;
    }
  } else if (sv->event->flags & CCM_EVENT_FLAG_CONNECT) {
    if (event->masks & CCM_EVENT_WRITE) {
      sv->event->flags &= (~((ushort_int)CCM_EVENT_FLAG_CONNECT));
      type = CCM_MESSAGE_CONNRSP;
    }
  } else {
    if (sv->event->masks == 0) {
      type = CCM_MESSAGE_IOEVENT;
    }
    sv->event->masks |= event->masks;
  }
  ll_unlock_event(sv);

  switch (type) {
  case CCM_MESSAGE_CONNIND:
    ccsocket_accept(sv->event->fd, ll_accept_connection, sv);
    break;
  case CCM_MESSAGE_CONNRSP:
    ll_service_send_message_um(sv, sv->svid, CCM_MESSAGE_CONNRSP, 0);
    break;
  case CCM_MESSAGE_IOEVENT:
    ll_service_send_message_um(sv, sv->svid, CCM_MESSAGE_IOEVENT, 0);
    break;
  }
}

void ccmaster_start() {
  struct ccsqueue queue;
  struct ccsqueue svmsgs;
  struct ccsqueue freeq;
  struct ccmessage* msg = 0;
  struct ccservice* sv = 0;
  struct ccthread* master = ccthread_getmaster();

  /* read parameters from config file and start worker threads */

  for (; ;) {
    ccionfmgr_wait(ll_get_ionfmgr(), ccmaster_dispatch_event);

    /* get all received messages */
    queue = ll_get_current_msgs();

    /* prepare master's messages */
    ccsqueue_init(&svmsgs);
    while ((msg = (struct ccmessage*)ccsqueue_pop(&queue))) {
      if (msg->dstid == CCM_SERVICE_MASTERID) {
        ccsqueue_push(&master->msgq, &msg->node);
      } else {
        ccsqueue_push(&svmsgs, &msg->node);
      }
    }

    /* handle master's messages */
    ccsqueue_init(&freeq);
    while ((msg = (struct ccmessage*)ccsqueue_pop(&master->msgq))) {
      switch (msg->type) {
      case LL_MESSAGE_RUNSERVICE:
        sv = (struct ccservice*)msg->data.ptr;
        if (sv->event) {
          ccionfmgr_add(ll_get_ionfmgr(), sv->event);
        }
        ll_add_service_to_table(sv);
        break;
      case LL_MESSAGE_ADDEVENT:
        if (msg->data.ptr) {
          struct ccioevent* event = (struct ccioevent*)msg->data.ptr;
          ccionfmgr_add(ll_get_ionfmgr(), event);
        }
        break;
      case LL_MESSAGE_DELEVENT:
        if (msg->data.ptr) {
          struct ccioevent* event = (struct ccioevent*)msg->data.ptr;
          ccionfmgr_del(ll_get_ionfmgr(), event);
          /* need free the event after it is deleted */
          ll_free_event(event);
        }
        break;
      case LL_MESSAGE_DELSERVICE:
        sv = (struct ccservice*)msg->data.ptr;
        /* service's received msgs (if any) */
        ccsqueue_pushqueue(&freeq, &sv->rxmq);
        /* detach and free event if any */
        if (sv->event) {
          ccionfmgr_del(ll_get_ionfmgr(), sv->event);
          ll_free_event(sv->event);
          sv->event = 0;
        }
        /* remove service out from hash table and insert to free queue */
        ll_service_is_finished(sv);
        /* reset the service */
        sv->belong = 0;
        break;
      default:
        ccloge("unknow message id");
        break;
      }
      ccsqueue_push(&freeq, &msg->node);
    }

    /* dispatch service's messages */
    while ((msg = (struct ccmessage*)ccsqueue_pop(&svmsgs))) {
      nauty_bool newattach = false;

      /* get message's dest service */
      sv = ll_find_service(msg->dstid);
      if (sv == 0) {
        cclogw("service not found");
        ccsqueue_push(&freeq, &msg->node);
        continue;
      }

      if (sv->belong) {
        nauty_bool done = false;

        ll_lock_thread(sv->belong);
        done = (sv->outflag & LL_SERVICE_COMPLETED) != 0;
        ll_unlock_thread(sv->belong);

        if (done) {
          /* free current msg and service's received msgs (if any) */
          ccsqueue_push(&freeq, &msg->node);
          ccsqueue_pushqueue(&freeq, &sv->rxmq);

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
        ccdqueue_push(&sv->belong->workrxq, &sv->node);
      }
      msg->extra = sv;
      ccsqueue_push(&sv->rxmq, &msg->node);
      sv->belong->missionassigned = true;
      ll_unlock_thread(sv->belong);
      /* signal the thread to handle */
      cccondv_signal(&sv->belong->condv);
    }

    ll_free_msgs(&freeq);
  }
}

void ccworker_start() {
  struct ccdqueue doneq;
  struct ccsqueue freemq;
  struct ccservice* service = 0;
  struct cclinknode* head = 0;
  struct cclinknode* elem = 0;
  struct ccmessage* msg = 0;
  struct ccthread* thread = ccthread_getself();

  for (; ;) {
    ccmutex_lock(&thread->mutex);
    while (!thread->missionassigned) {
      cccondv_wait(&thread->condv, &thread->mutex);
    }
    /* mission waited, reset false */
    thread->missionassigned = false;
    ccdqueue_pushqueue(&thread->workq, &thread->workrxq);
    ccmutex_unlock(&thread->mutex);

    head = &(thread->workq.head);
    for (elem = head->next; elem != head; elem = elem->next) {
      service = (struct ccservice*)elem;
      if (!ccsqueue_isempty(&service->rxmq)) {
        ll_lock_thread(thread);
        ccsqueue_pushqueue(&thread->msgq, &service->rxmq);
        ll_unlock_thread(thread);
      }
    }

    if (ccsqueue_isempty(&thread->msgq)) {
      /* there are no messages received */
      continue;
    }

    ccdqueue_init(&doneq);
    ccsqueue_init(&freemq);
    while ((msg = (struct ccmessage*)ccsqueue_pop(&thread->msgq))) {
      service = (struct ccservice*)msg->extra;
      if (service->flag & LL_SERVICE_COMPLETED) {
        ccsqueue_push(&freemq, &msg->node);
        continue;
      }

      if (msg->type == CCM_MESSAGE_IOEVENT) {
        ll_lock_thread(thread);
        msg->data.u32 = service->event->masks;
        service->event->masks = 0;
        ll_unlock_thread(thread);
        if (msg->data.u32 == 0) {
          ccsqueue_push(&freemq, &msg->node);
          continue;
        }
      }

#if 0
      if (service->flag & LL_SERVICE_YIELDABLE) {
        if (service->co == 0) {
          service->co = ll_new_coroutine(thread, service);
        }
        if (service->co->L != thread->L) {
          ccloge("wrong lua state");
          ccsqueue_push(&freemq, &msg->node);
          continue;
        }
        service->co->srvc = service;
        service->co->msg = msg;
        ccstate_resume(service->co);
      } else {

      }
#endif

      service->entry(service, msg);

      /* service run completed */
      if (service->flag & LL_SERVICE_COMPLETED) {
        ccdqueue_push(&doneq, cclinknode_remove(&service->node));
        if (service->co) {
          ll_free_coroutine(thread, service->co);
          service->co = 0;
        }
      }

      /* free handled message */
      ccsqueue_push(&freemq, &msg->node);
    }

    while ((service = (struct ccservice*)ccdqueue_pop(&doneq))) {
      ll_lock_thread(thread);
      service->outflag |= LL_SERVICE_COMPLETED;
      ll_unlock_thread(thread);

      /* currently the finished service is removed out from thread's
      service queue, but the service still pointer to this thread.
      send SVDONE message to master to reset and free this service. */
      ll_service_send_message_sp(service, CCM_SERVICE_MASTERID, LL_MESSAGE_DELSERVICE, service);
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
  struct ccglobal G;
  struct ccthread master;

  /* init global first */
  ccglobal_init(&G);

  /* init master thread */
  ccthread_init(&master);
  master.id = ccplat_selfthread();

  /* attach master thread */
  ccglobal_setmaster(&master);

  /* call start func */
  ll_parse_command_line(argc, argv);
  n = start();

  /* clean */
  ccthread_free(&master);
  ccglobal_free(&G);
  return n;
}

int startmainthread(int (*start)()) {
  return startmainthreadcv(start, 0, 0);
}

