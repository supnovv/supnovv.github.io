#include <stddef.h>
#include "service.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"

#define LLSERVICE_START_SVID 0x2001
#define LLSERVICE_INITSIZEBITS 8 /* 256 */
#define CCSERVICE_DONE 0x01
#define CCSERVICE_SAMETHREAD 0x02
#define CCSERVICE_FUNC 0x10
#define CCSERVICE_CCCO 0x20
#define CCSERVICE_LUAF 0x40

#define ROBOT_COMPLETED  0x01
#define ROBOT_NOT_START  0x02
#define ROBOT_SAMETHREAD 0x04
#define ROBOT_YIELDABLE  0x08

struct ccrobot {
  /* shared with master */
  struct cclinknode node;
  struct ccsmplnode tlink;
  struct ccthread* belong; /* only modified by master when init */
  struct ccsqueue rxmq;
  ushort_int outflag;
  /* thread own use */
  ushort_int flag;
  umedit_int svid;
  struct ccioevent* event;
  struct ccstate* co;
  int (*func)(struct ccstate*);
  void* udata;
};

static void ccrobot_init(struct ccrobot* self) {
  cczeron(self, sizeof(struct ccrobot));
  cclinknode_init(&self->node);
  ccsmplnode_init(&self->tlink);
  ccsqueue_init(&self->rxmq);
}

static umedit_int ll_service_getkey(void* service) {
  return ((struct ccrobot*)service)->svid;
}

static int ll_service_tlink_offset() {
  return offsetof(struct ccrobot, tlink);
}

struct ccstate* ccrobot_getstate(struct ccrobot* self) {
  return self->co;
}

void* robot_get_specific(struct ccstate* state) {
  return state->srvc->udata;
}

void* robot_set_specific(struct ccrobot* robot, void* udata) {
  robot->udata = udata;
  return udata;
}

void* robot_set_allocated_specific(struct ccrobot* robot, int bytes) {
  robot->udata = ccrawalloc(bytes);
  return robot->udata;
}

struct ccmessage* robot_get_message(struct ccstate* state) {
  return state->msg;
}

struct ccthread {
  /* access by master only */
  struct cclinknode node;
  umedit_int weight;
  /* shared with master */
  nauty_bool missionassigned;
  struct ccdqueue workrxq;
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
};

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

  ccmutex_init(&self->mutex);
  cccondv_init(&self->condv);

  self->L = cclua_newstate();
  self->defstr = ccemptystr();
  self->index = ll_new_thread_index();
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

  ccmutex_free(&self->mutex);
  cccondv_free(&self->condv);
  ccstring_free(&self->defstr);
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
  ccbackhash_init(&self->table, initsizebits, ll_service_tlink_offset(), ll_service_getkey);
  ccdqueue_init(&self->freeq);
  ccmutex_init(&self->mutex);
  self->freesize = 0;
  self->seed = LLSERVICE_START_SVID;
}

static void ll_service_free_memory(void* service) {
  ccrawfree(service);
}

static void llservices_free(struct llservices* self) {
  struct ccrobot* sv = 0;
  cchashtable_foreach(&self->table.a, ll_service_free_memory);
  cchashtable_foreach(&self->table.b, ll_service_free_memory);

  while ((sv = (struct ccrobot*)ccdqueue_pop(&self->freeq))) {
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

static void ll_add_service_to_table(struct ccrobot* srvc) {
  struct llservices* self = ll_ccg_services();
  ccmutex_lock(&self->mutex);
  ccbackhash_add(&self->table, srvc);
  ccmutex_unlock(&self->mutex);
}

static struct ccrobot* ll_find_service(umedit_int svid) {
  struct llservices* self = ll_ccg_services();
  struct ccrobot* srvc = 0;

  ccmutex_lock(&self->mutex);
  srvc = (struct ccrobot*)ccbackhash_find(&self->table, svid);
  ccmutex_unlock(&self->mutex);

  return srvc;
}

static void ll_service_is_finished(struct ccrobot* srvc) {
  struct llservices* self = ll_ccg_services();
  ll_thread_finish_a_service(srvc->belong);

  ccmutex_lock(&self->mutex);
  ccbackhash_del(&self->table, srvc->svid);
  ccdqueue_push(&self->freeq, &srvc->node);
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

  ccmutex_lock(&self->mutex);
  ccsqueue_push(&self->freeq, &event->node);
  self->freesize += 1;
  ccmutex_unlock(&self->mutex);
}

void ccrobot_detach_event(struct ccrobot* self) {
  struct ccionfmgr* mgr = ccgetionfmgr();
  if (self->event && ccsocket_isopen(self->event->fd) && (self->event->flags & CCIOFLAG_ADDED)) {
    self->event->flags &= (~(ushort_int)CCIOFLAG_ADDED);
    ccionfmgr_del(mgr, self->event);
  }
}

void ccrobot_attach_event(struct ccrobot* self, handle_int fd, ushort_int masks, ushort_int flags) {
  ccrobot_detach_event(self);
  if (!self->event) {
    self->event = ll_new_event();
  }
  self->event->fd = fd;
  self->event->udata = self->svid;
  self->event->masks = (masks | IOEVENT_ERR);
  self->event->flags = flags;
  ccionfmgr_add(ccgetionfmgr(), self->event);
  self->event->flags |= IOEVENT_FLAG_ADDED;
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
  struct ccionfmgr* ionf = ccgetionfmgr();
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
  if (self->data.ptr && (self->flag & CCMSGFLAG_FREEPTR)) {
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
  llservices_init(&self->services, LLSERVICE_INITSIZEBITS);
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

struct ccionfmgr* ccgetionfmgr() {
  return &(ccG->ionf);
}


/**
 * thread
 */

static struct ccstate* ll_new_coroutine(struct ccthread* thread, struct ccrobot* srvc) {
  struct ccstate* co = 0;
  if ((co = (struct ccstate*)ccsqueue_pop(&thread->freeco))) {
    thread->freecosz -= 1;
  } else {
    co = (struct ccstate*)ccrawalloc(sizeof(struct ccstate));
    thread->totalco += 1;
  }
  ccstate_init(co, thread->L, srvc->func, srvc);
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

static struct ccrobot* ll_new_service(umedit_int svid) {
  struct llservices* self = ll_ccg_services();
  struct ccrobot* srvc = 0;

  ccmutex_lock(&self->mutex);
  srvc = (struct ccrobot*)ccdqueue_pop(&self->freeq);
  if (self->freesize > 0) {
    self->freesize -= 1;
  }
  ccmutex_unlock(&self->mutex);

  if (srvc == 0) {
    srvc = (struct ccrobot*)ccrawalloc(sizeof(struct ccrobot));
    self->total += 1;
  }

  ccrobot_init(srvc);
  if (svid == 0) {
    srvc->svid = ll_new_service_id();
  } else {
    srvc->svid = svid;
  }

  return srvc;
}

struct ccrobot* ccrobot_new(void* udata, int (*func)(struct ccstate*)) {
  struct ccrobot* srvc = ll_new_service(0);
  struct ccmessage* msg = 0;
  srvc->udata = udata;
  srvc->func = func;
  msg = ccnewmessage(0, CCMSGID_ADDSRVC);
  msg->data.ptr = srvc;
  ccrobot_sendmsgtomaster(srvc, msg);
  return srvc;
}

struct ccrobot* ccrobot_newfrom(umedit_int svid, void* udata, int (*func)(struct ccstate*)) {
  struct ccrobot* sv = ll_new_service(0);
  struct ccmessage* msg = 0;
  sv->udata = udata;
  sv->func = func;
  sv->flag = CCSERVICE_SAMETHREAD;
  msg = ccnewmessage(0, CCMSGID_ADDSRVC);
  msg->data.ptr = sv;
  msg->flag = svid;
  ccrobot_sendmsgtomaster(sv, msg);
  return sv;
}

struct ccrobot* robot_create_new(int (*func)(struct ccstate*), int yieldable) {
  struct ccrobot* robot = ll_new_service(0);
  robot->func = func;
  robot->flag = (yieldable == ROBOT_YIELDABLE ? ROBOT_YIELDABLE : 0);
  robot->flag |= ROBOT_NOT_START;
  return robot;
}

struct ccrobot* robot_create_from(struct ccstate* state, int (*func)(struct ccstate*), int yieldable) {
  struct ccrobot* robot = robot_create_new(func, yieldable);
  robot->belong = state->srvc->belong;
  robot->flag |= ROBOT_SAMETHREAD;
  return robot;
}

#if 0
void robot_change_listen(struct ccstate* state, handle_int fd, ushort_int masks, ushort_int flags) {

}

void robot_remove_listen(struct ccstate* state) {
  if (robot->flag & ROBOT_NOT_START) {
    ll_free_event(robot->event);
    robot->event = 0;
  } else {

  }
}
#endif

void ll_set_event(struct ccioevent* event, handle_int fd, umedit_int udata, ushort_int masks, ushort_int flags) {
  event-> fd = fd;
  event->udata = udata;
  event->masks = (masks | IOEVENT_ERR);
  event->flags = flags;
}

void robot_set_listen(struct ccrobot* robot, handle_int fd, ushort_int masks, ushort_int flags) {
  if (robot->flag & ROBOT_NOT_START) {
    ll_free_event(robot->event);
    robot->event = ll_new_event();
    ll_set_event(robot->event, fd, robot->svid, masks, flags);
  }
}

void robot_start_run(struct ccrobot* robot) {
  if (robot->flag & ROBOT_NOT_START) {

  }
}

void robot_run_completed(struct ccstate* state) {
  /* robot->flag is accessed by thread itself */
  state->srvc->flag |= ROBOT_COMPLETED;
}

void robot_yield(struct ccstate* state, int (*kfunc)(struct ccstate*)) {
  ccstate_yield(state, kfunc);
}

struct ccmessage* ccnewmessage(umedit_int destsvid, umedit_int type) {
  struct ccmessage* msg = ll_new_msg();
  msg->dstid = destsvid;
  msg->type = type;
  msg->flag = 0;
  return msg;
}

struct ccmessage* ccnewmessage_allocated(umedit_int destsvid, umedit_int type, void* ptr) {
  struct ccmessage* msg = ll_new_msg();
  msg->dstid = destsvid;
  msg->type = type;
  msg->flag = CCMSGFLAG_FREEPTR;
  msg->data.ptr = ptr;
  return msg;
}

void ccrobot_sendmsg(struct ccrobot* self, struct ccmessage* msg) {
  msg->srcid = self->svid;
  ll_send_message(msg);
}

void ccrobot_sendmsgtomaster(struct ccrobot* self, struct ccmessage* msg) {
  msg->srcid = self->svid;
  msg->dstid = 0;
  ll_send_message(msg);
}

void ccrobot_sendtomaster(struct ccrobot* self, umedit_int type) {
  struct ccmessage* msg = 0;
  msg = ccnewmessage(0, type);
  msg->data.ptr = self;
  ccrobot_sendmsgtomaster(self, msg);
}

static void ll_accept_connection(void* ud, struct ccsockconn* conn) {
  struct ccrobot* sv = (struct ccrobot*)ud;
  struct ccmessage* msg = ccnewmessage(sv->svid, CCMSGID_CONNIND);
  msg->data.fd = conn->sock;
  ccrobot_sendmsg(sv, msg);
}

static void ccmaster_dispatch_event(struct ccioevent* event) {
  umedit_int svid = event->udata;
  struct ccrobot* sv = 0;
  struct ccmessage* msg = 0;
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

  ll_lock_thread(sv->belong);
  if (sv->event->flags & IOEVENT_FLAG_LISTEN) {
    if (event->masks & CCIONFRD) {
      type = CCMSGID_CONNIND;
    }
  } else if (sv->event->flags & IOEVENT_FLAG_CONNECT) {
    if (event->masks & CCIONFWR) {
      sv->event->flags &= (~((ushort_int)IOEVENT_FLAG_CONNECT));
      type = CCMSGID_CONNRSP;
    }
  } else {
    if (sv->event->masks == 0) {
      type = CCMSGID_IOEVENT;
    }
    sv->event->masks |= event->masks;
  }
  ll_unlock_thread(sv->belong);

  switch (type) {
  case CCMSGID_CONNIND:
    ccsocket_accept(sv->event->fd, ll_accept_connection, sv);
    break;
  case CCMSGID_CONNRSP:
    msg = ccnewmessage(sv->svid, CCMSGID_CONNRSP);
    ccrobot_sendmsg(sv, msg);
    break;
  case CCMSGID_IOEVENT:
    msg = ccnewmessage(sv->svid, CCMSGID_IOEVENT);
    ccrobot_sendmsg(sv, msg);
    break;
  }
}

void ccmaster_start() {
  struct ccsqueue queue;
  struct ccsqueue svmsgs;
  struct ccsqueue freeq;
  struct ccmessage* msg = 0;
  struct ccrobot* sv = 0;
  struct ccthread* master = ccthread_getmaster();

  /* read parameters from config file and start worker threads */

  for (; ;) {
    ccionfmgr_wait(ccgetionfmgr(), ccmaster_dispatch_event);

    /* get all received messages */
    queue = ll_get_current_msgs();

    /* prepare master's messages */
    ccsqueue_init(&svmsgs);
    while ((msg = (struct ccmessage*)ccsqueue_pop(&queue))) {
      if (msg->dstid == 0) {
        ccsqueue_push(&master->msgq, &msg->node);
      } else {
        ccsqueue_push(&svmsgs, &msg->node);
      }
    }

    /* handle master's messages */
    ccsqueue_init(&freeq);
    while ((msg = (struct ccmessage*)ccsqueue_pop(&master->msgq))) {
      sv = (struct ccrobot*)msg->data.ptr;
      switch (msg->type) {
      case CCMSGID_ADDSRVC:
        if (sv->flag & CCSERVICE_SAMETHREAD) {
          umedit_int fromsvid = msg->flag;
          struct ccrobot* fromsrvc = ll_find_service(fromsvid);
          sv->belong = fromsrvc->belong;
        }
        ll_add_service_to_table(sv);
        break;
      case CCMSGID_DELSRVC:
        /* service's received msgs (if any) */
        ccsqueue_pushqueue(&freeq, &sv->rxmq);
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
        done = (sv->outflag & CCSERVICE_DONE) != 0;
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
  struct ccrobot* robot = 0;
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
      robot = (struct ccrobot*)elem;
      if (!ccsqueue_isempty(&robot->rxmq)) {
        ll_lock_thread(thread);
        ccsqueue_pushqueue(&thread->msgq, &robot->rxmq);
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
      robot = (struct ccrobot*)msg->extra;
      if (robot->flag & ROBOT_COMPLETED) {
        ccsqueue_push(&freemq, &msg->node);
        continue;
      }

      if (msg->type == CCMSGID_IOEVENT) {
        ll_lock_thread(thread);
        msg->data.us = robot->event->masks;
        robot->event->masks = 0;
        ll_unlock_thread(thread);
        if (msg->data.us == 0) {
          ccsqueue_push(&freemq, &msg->node);
          continue;
        }
      }

      if (robot->flag & CCSERVICE_CCCO) {
        if (robot->co == 0) {
          robot->co = ll_new_coroutine(thread, robot);
        }
        if (robot->co->L != thread->L) {
          ccloge("wrong lua state");
          ccsqueue_push(&freemq, &msg->node);
          continue;
        }
        robot->co->srvc = robot;
        robot->co->msg = msg;
        ccstate_resume(robot->co);
        robot->co->srvc = 0;
        robot->co->msg = 0;
      } else if (robot->flag & CCSERVICE_FUNC) {

      } else if (robot->flag & CCSERVICE_LUAF) {

      }
      else {
        ccloge("invalide service type");
      }

      /* service run completed */
      if (robot->flag & ROBOT_COMPLETED) {
        ccdqueue_push(&doneq, cclinknode_remove(&robot->node));
        if (robot->co) {
          ll_free_coroutine(thread, robot->co);
          robot->co = 0;
        }
      }

      /* free handled message */
      ccsqueue_push(&freemq, &msg->node);
    }

    while ((robot = (struct ccrobot*)ccdqueue_pop(&doneq))) {
      ll_lock_thread(thread);
      robot->outflag |= ROBOT_COMPLETED;
      ll_unlock_thread(thread);

      /* currently the finished service is removed out from thread's
      service queue, but the service still pointer to this thread.
      send SVDONE message to master to reset and free this service. */
      ccrobot_sendtomaster(robot, CCMSGID_DELSRVC);
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

