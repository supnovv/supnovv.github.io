#include <stddef.h>
#include "service.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"

#define LLSERVICE_START_SVID 0x2001
#define LLSERVICE_INITSIZEBITS 8 /* 256 */

#define MESSAGE_RUNROBOT 0x01
#define MESSAGE_DELROBOT 0x02
#define MESSAGE_ADDEVENT 0x03
#define MESSAGE_DELEVENT 0x04
#define MESSAGE_FLAG_FREEPTR 0x01

#define ROBOT_COMPLETED  0x01
#define ROBOT_NOT_START  0x02
#define ROBOT_SAMETHREAD 0x04
#define ROBOT_YIELDABLE  0x08
#define ROBOT_FREE_DATA  0x10

struct ccrobot {
  /* shared with master */
  struct cclinknode node;
  struct ccsmplnode tlink;
  struct ccsqueue rxmq;
  struct ccioevent* event;
  ushort_int outflag;
  /* thread own use */
  ushort_int flag;
  umedit_int roid;
  struct ccthread* belong; /* set once when init */
  struct ccstate* co;
  int (*entry)(struct ccrobot*, struct ccmessage*);
  void* udata;
};

static void ccrobot_init(struct ccrobot* self) {
  cczeron(self, sizeof(struct ccrobot));
  cclinknode_init(&self->node);
  ccsmplnode_init(&self->tlink);
  ccsqueue_init(&self->rxmq);
}

static int ll_robot_tlink_offset() {
  return offsetof(struct ccrobot, tlink);
}

struct ccstate* ccrobot_getstate(struct ccrobot* self) {
  return self->co;
}

umedit_int robot_get_id(struct ccrobot* robot) {
  return robot->roid;
}

void* robot_get_specific(struct ccstate* state) {
  return state->bot->udata;
}

struct ccmessage* robot_get_message(struct ccstate* state) {
  return state->msg;
}

handle_int robot_get_eventfd(struct ccstate* state) {
  return state->bot->event->fd;
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

  ccmutex_init(&self->elock);
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

  /* un-complete robots are all in the hash table,
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
static struct ccthread* ll_get_thread_for_new_robot() {
  struct llthreads* self = ll_ccg_threads();
  struct ccthread* thread = (struct ccthread*)ccpriorq_pop(&self->priq);
  thread->weight += 1;
  ccpriorq_push(&self->priq, &thread->node);
  return thread;
}

/* no protacted, can only be called by master */
static void ll_thread_finish_a_robot(struct ccthread* thread) {
  struct llthreads* self = ll_ccg_threads();
  if (thread->weight > 0) {
    thread->weight -= 1;
    ccpriorq_remove(&self->priq, &thread->node);
    ccpriorq_push(&self->priq, &thread->node);
  }
}


/**
 * robots container
 */

struct llrobots {
  struct ccbackhash table;
  struct ccdqueue freeq;
  umedit_int freesize;
  umedit_int total;
  umedit_int seed;
  struct ccmutex mutex;
};

static struct llrobots* ll_ccg_robots();

static void llrobots_init(struct llrobots* self, nauty_byte initsizebits) {
  ccbackhash_init(&self->table, initsizebits, ll_robot_tlink_offset(), (umedit_int(*)(void*))robot_get_id);
  ccdqueue_init(&self->freeq);
  ccmutex_init(&self->mutex);
  self->freesize = 0;
  self->seed = LLSERVICE_START_SVID;
}

static void ll_robot_free_memory(void* ro) {
  struct ccrobot* robot = (struct ccrobot*)ro;
  if (robot->udata && (robot->flag & ROBOT_FREE_DATA)) {
    ccrawfree(robot->udata);
    robot->udata = 0;
  }
  ccrawfree(robot);
}

static void llrobots_free(struct llrobots* self) {
  struct ccrobot* ro = 0;
  cchashtable_foreach(&self->table.a, ll_robot_free_memory);
  cchashtable_foreach(&self->table.b, ll_robot_free_memory);

  while ((ro = (struct ccrobot*)ccdqueue_pop(&self->freeq))) {
    ll_robot_free_memory(ro);
  }

  ccbackhash_free(&self->table);
  ccmutex_free(&self->mutex);
}

static umedit_int ll_new_robot_id() {
  struct llrobots* self = ll_ccg_robots();
  umedit_int roid = 0;

  ccmutex_lock(&self->mutex);
  roid = self->seed++;
  ccmutex_unlock(&self->mutex);

  return roid;
}

static void ll_add_robot_to_table(struct ccrobot* robot) {
  struct llrobots* self = ll_ccg_robots();
  ccmutex_lock(&self->mutex);
  ccbackhash_add(&self->table, robot);
  ccmutex_unlock(&self->mutex);
}

static struct ccrobot* ll_find_robot(umedit_int roid) {
  struct llrobots* self = ll_ccg_robots();
  struct ccrobot* robot = 0;

  ccmutex_lock(&self->mutex);
  robot = (struct ccrobot*)ccbackhash_find(&self->table, roid);
  ccmutex_unlock(&self->mutex);

  return robot;
}

static void ll_robot_is_finished(struct ccrobot* robot) {
  struct llrobots* self = ll_ccg_robots();
  ll_thread_finish_a_robot(robot->belong);

  ccmutex_lock(&self->mutex);
  ccbackhash_del(&self->table, robot->roid);
  ccdqueue_push(&self->freeq, &robot->node);
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

static void ll_lock_event(struct ccrobot* robot) {
  ccmutex_lock(&robot->belong->elock);
}

static void ll_unlock_event(struct ccrobot* robot) {
  ccmutex_unlock(&robot->belong->elock);
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

void ccrobot_detach_event(struct ccrobot* self) {
  struct ccionfmgr* mgr = ll_get_ionfmgr();
  if (self->event && ccsocket_isopen(self->event->fd) && (self->event->flags & CCIOFLAG_ADDED)) {
    self->event->flags &= (~CCIOFLAG_ADDED);
    ccionfmgr_del(mgr, self->event);
  }
}

void ccrobot_attach_event(struct ccrobot* self, handle_int fd, ushort_int masks, ushort_int flags) {
  ccrobot_detach_event(self);
  if (!self->event) {
    self->event = ll_new_event();
  }
  self->event->fd = fd;
  self->event->udata = self->roid;
  self->event->masks = (masks | IOEVENT_ERR);
  self->event->flags = flags;
  ccionfmgr_add(ll_get_ionfmgr(), self->event);
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
  if (self->data.ptr && (self->flag & MESSAGE_FLAG_FREEPTR)) {
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
  struct llrobots robots;
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

static struct llrobots* ll_ccg_robots() {
  return &ccG->robots;
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
  llrobots_init(&self->robots, LLSERVICE_INITSIZEBITS);
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
  llrobots_free(&self->robots);
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

static struct ccstate* ll_new_state(struct ccrobot* bot, int (*func)(struct ccstate*)) {
  struct ccstate* co;
  struct ccthread* thr = bot->belong;
  if ((co = (struct ccstate*)ccsqueue_pop(&thr->freeco))) {
    thr->freecosz --;
  } else {
    co = (struct ccstate*)ccrawalloc(sizeof(struct ccstate));
    thr->totalco += 1;
  }
  ccstate_init(co, thr->L, func, bot);
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
 * robot
 */

static struct ccrobot* ll_new_robot(umedit_int roid) {
  struct llrobots* self = ll_ccg_robots();
  struct ccrobot* robot = 0;

  ccmutex_lock(&self->mutex);
  robot = (struct ccrobot*)ccdqueue_pop(&self->freeq);
  if (self->freesize > 0) {
    self->freesize -= 1;
  }
  ccmutex_unlock(&self->mutex);

  if (robot == 0) {
    robot = (struct ccrobot*)ccrawalloc(sizeof(struct ccrobot));
    self->total += 1;
  }

  ccrobot_init(robot);
  if (roid == 0) {
    robot->roid = ll_new_robot_id();
  } else {
    robot->roid = roid;
  }

  return robot;
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
  self->flag |= MESSAGE_FLAG_FREEPTR;
  self->data.ptr = ccrawalloc(bytes);
  return self->data.ptr;
}

static void ll_robot_send_message(struct ccrobot* robot, umedit_int destid, struct ccmessage* msg) {
  msg->srcid = robot->roid;
  msg->dstid = destid;
  ll_send_message(msg);
}

static void ll_robot_send_message_sp(struct ccrobot* robot, umedit_int destid, umedit_int type, void* static_ptr) {
  struct ccmessage* msg = message_create(type, 0);
  message_set_specific(msg, static_ptr);
  ll_robot_send_message(robot, destid, msg);
}

static void ll_robot_send_message_um(struct ccrobot* robot, umedit_int destid, umedit_int type, umedit_int data) {
  struct ccmessage* msg = message_create(type, 0);
  msg->data.um = data;
  ll_robot_send_message(robot, destid, msg);
}

static void ll_robot_send_message_fd(struct ccrobot* robot, umedit_int destid, umedit_int type, handle_int fd) {
  struct ccmessage* msg = message_create(type, 0);
  msg->data.fd = fd;
  ll_robot_send_message(robot, destid, msg);
}

void robot_send_message(struct ccstate* state, umedit_int destid, struct ccmessage* msg) {
  ll_robot_send_message(state->bot, destid, msg);
}

void robot_send_message_sp(struct ccstate* state, umedit_int destid, umedit_int type, void* static_ptr) {
  ll_robot_send_message_sp(state->bot, destid, type, static_ptr);
}

void robot_send_message_um(struct ccstate* state, umedit_int destid, umedit_int type, umedit_int data) {
  ll_robot_send_message_um(state->bot, destid, type, data);
}

void robot_send_message_fd(struct ccstate* state, umedit_int destid, umedit_int type, handle_int fd) {
  ll_robot_send_message_fd(state->bot, destid, type, fd);
}

#if 0
struct ccrobot* robot_newfromlua(const void* chunk, int len) {

}
#endif

struct ccrobot* robot_new(int (*entry)(struct ccrobot*, struct ccmessage*)) {
  struct ccrobot* robot = ll_new_robot(0);
  robot->entry = entry;
  robot->flag = ROBOT_NOT_START;
  return robot;
}

#if 0
struct ccrobot* robot_create(int (*entry)(struct ccstate*), int yieldable) {
  struct ccrobot* robot = ll_new_robot(0);
  robot->entry = entry;
  robot->flag = (yieldable == ROBOT_YIELDABLE ? ROBOT_YIELDABLE : 0);
  robot->flag |= ROBOT_NOT_START;
  return robot;
}

struct ccrobot* robot_create_from(struct ccstate* state, void (*func)(struct ccstate*), int yieldable) {
  struct ccrobot* robot = robot_create(func, yieldable);
  robot->belong = state->bot->belong;
  robot->flag |= ROBOT_SAMETHREAD;
  return robot;
}
#endif

void* robot_set_specific(struct ccrobot* robot, void* udata) {
  robot->udata = udata;
  return udata;
}

void* robot_set_allocated_specific(struct ccrobot* robot, int bytes) {
  robot->udata = ccrawalloc(bytes);
  robot->flag |= ROBOT_FREE_DATA;
  return robot->udata;
}

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
    ll_set_event(robot->event, fd, robot->roid, masks, flags);
  }
}

void robot_listen_event(struct ccstate* state, handle_int fd, ushort_int masks, ushort_int flags) {
  struct ccrobot* robot = state->bot;
  struct ccioevent* old = 0;

  ll_lock_event(robot);
  old = robot->event;
  robot->event = ll_new_event();
  ll_set_event(robot->event, fd, robot->roid, masks, flags);  
  ll_unlock_event(robot);

  if (old) {
    /* need free this old event after delete operation */
    ll_robot_send_message_sp(robot, ROBOT_ID_MASTER, MESSAGE_DELEVENT, old);
  }
  ll_robot_send_message_sp(robot, ROBOT_ID_MASTER, MESSAGE_ADDEVENT, robot->event);
}

void robot_remove_listen(struct ccstate* state) {
  struct ccrobot* robot = state->bot;
  struct ccioevent* old = 0;

  ll_lock_event(robot);
  old = robot->event;
  robot->event = 0;
  ll_unlock_event(robot);

  if (old) {
    /* need free this old event after delete operation */
    ll_robot_send_message_sp(robot, ROBOT_ID_MASTER, MESSAGE_DELEVENT, old);
  }
}

void robot_start_run(struct ccrobot* robot) {
  if (robot->flag & ROBOT_NOT_START) {
    ll_robot_send_message_sp(robot, ROBOT_ID_MASTER, MESSAGE_RUNROBOT, robot);
    robot->flag &= (~ROBOT_NOT_START);
  }
}

void robot_run_completed(struct ccstate* state) {
  /* robot->flag is accessed by thread itself */
  state->bot->flag |= ROBOT_COMPLETED;
}

void robot_yield(struct ccstate* state, int (*kfunc)(struct ccstate*)) {
  ccstate_yield(state, kfunc);
}

void robot_resume(struct ccrobot* robot, int (*func)(struct ccstate*)) {
  if (robot->co == 0) {
    robot->co = ll_new_state(robot, func);
  }
  robot->co->bot = robot;
  robot->co->func = func;
  ccstate_resume(robot->co);
}

static void ll_accept_connection(void* ud, struct ccsockconn* conn) {
  struct ccrobot* ro = (struct ccrobot*)ud;
  ll_robot_send_message_fd(ro, ro->roid, MESSAGE_CONNIND, conn->sock);
}

static void ccmaster_dispatch_event(struct ccioevent* event) {
  umedit_int roid = event->udata;
  struct ccrobot* ro = 0;
  umedit_int type = 0;

  ro = ll_find_robot(roid);
  if (ro == 0) {
    ccloge("robot not found");
    return;
  }

  if (event->fd != ro->event->fd) {
    ccloge("fd mismatch");
    return;
  }

  ll_lock_event(ro);
  if (ro->event->flags & IOEVENT_FLAG_LISTEN) {
    if (event->masks & CCIONFRD) {
      type = MESSAGE_CONNIND;
    }
  } else if (ro->event->flags & IOEVENT_FLAG_CONNECT) {
    if (event->masks & CCIONFWR) {
      ro->event->flags &= (~((ushort_int)IOEVENT_FLAG_CONNECT));
      type = MESSAGE_CONNRSP;
    }
  } else {
    if (ro->event->masks == 0) {
      type = MESSAGE_IOEVENT;
    }
    ro->event->masks |= event->masks;
  }
  ll_unlock_event(ro);

  switch (type) {
  case MESSAGE_CONNIND:
    ccsocket_accept(ro->event->fd, ll_accept_connection, ro);
    break;
  case MESSAGE_CONNRSP:
    ll_robot_send_message_um(ro, ro->roid, MESSAGE_CONNRSP, 0);
    break;
  case MESSAGE_IOEVENT:
    ll_robot_send_message_um(ro, ro->roid, MESSAGE_IOEVENT, 0);
    break;
  }
}

void ccmaster_start() {
  struct ccsqueue queue;
  struct ccsqueue romsgs;
  struct ccsqueue freeq;
  struct ccmessage* msg = 0;
  struct ccrobot* ro = 0;
  struct ccthread* master = ccthread_getmaster();

  /* read parameters from config file and start worker threads */

  for (; ;) {
    ccionfmgr_wait(ll_get_ionfmgr(), ccmaster_dispatch_event);

    /* get all received messages */
    queue = ll_get_current_msgs();

    /* prepare master's messages */
    ccsqueue_init(&romsgs);
    while ((msg = (struct ccmessage*)ccsqueue_pop(&queue))) {
      if (msg->dstid == ROBOT_ID_MASTER) {
        ccsqueue_push(&master->msgq, &msg->node);
      } else {
        ccsqueue_push(&romsgs, &msg->node);
      }
    }

    /* handle master's messages */
    ccsqueue_init(&freeq);
    while ((msg = (struct ccmessage*)ccsqueue_pop(&master->msgq))) {
      switch (msg->type) {
      case MESSAGE_RUNROBOT:
        ro = (struct ccrobot*)msg->data.ptr;
        if (ro->event) {
          ccionfmgr_add(ll_get_ionfmgr(), ro->event);
        }
        ll_add_robot_to_table(ro);
        break;
      case MESSAGE_ADDEVENT:
        if (msg->data.ptr) {
          struct ccioevent* event = (struct ccioevent*)msg->data.ptr;
          ccionfmgr_add(ll_get_ionfmgr(), event);
        }
        break;
      case MESSAGE_DELEVENT:
        if (msg->data.ptr) {
          struct ccioevent* event = (struct ccioevent*)msg->data.ptr;
          ccionfmgr_del(ll_get_ionfmgr(), event);
          /* need free the event after it is deleted */
          ll_free_event(event);
        }
        break;
      case MESSAGE_DELROBOT:
        ro = (struct ccrobot*)msg->data.ptr;
        /* robot's received msgs (if any) */
        ccsqueue_pushqueue(&freeq, &ro->rxmq);
        /* detach and free event if any */
        if (ro->event) {
          ccionfmgr_del(ll_get_ionfmgr(), ro->event);
          ll_free_event(ro->event);
          ro->event = 0;
        }
        /* remove robot out from hash table and insert to free queue */
        ll_robot_is_finished(ro);
        /* reset the robot */
        ro->belong = 0;
        break;
      default:
        ccloge("unknow message id");
        break;
      }
      ccsqueue_push(&freeq, &msg->node);
    }

    /* dispatch robot's messages */
    while ((msg = (struct ccmessage*)ccsqueue_pop(&romsgs))) {
      nauty_bool newattach = false;

      /* get message's dest robot */
      ro = ll_find_robot(msg->dstid);
      if (ro == 0) {
        cclogw("robot not found");
        ccsqueue_push(&freeq, &msg->node);
        continue;
      }

      if (ro->belong) {
        nauty_bool done = false;

        ll_lock_thread(ro->belong);
        done = (ro->outflag & ROBOT_COMPLETED) != 0;
        ll_unlock_thread(ro->belong);

        if (done) {
          /* free current msg and robot's received msgs (if any) */
          ccsqueue_push(&freeq, &msg->node);
          ccsqueue_pushqueue(&freeq, &ro->rxmq);

          /* when robot is done, the robot is already removed out from thread's robot queue.
          here delete robot from the hash table and insert into robot's free queue */
          ll_robot_is_finished(ro);

          /* reset the robot */
          ro->belong = 0;
          continue;
        }
      } else {
        /* attach a thread to handle the robot */
        ro->belong = ll_get_thread_for_new_robot();
        newattach = true;
      }

      /* queue the robot to its belong thread */
      ll_lock_thread(ro->belong);
      if (newattach) {
        ro->outflag = ro->flag = 0;
        ccdqueue_push(&ro->belong->workrxq, &ro->node);
      }
      msg->extra = ro;
      ccsqueue_push(&ro->rxmq, &msg->node);
      ro->belong->missionassigned = true;
      ll_unlock_thread(ro->belong);
      /* signal the thread to handle */
      cccondv_signal(&ro->belong->condv);
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

      if (msg->type == MESSAGE_IOEVENT) {
        ll_lock_thread(thread);
        msg->data.us = robot->event->masks;
        robot->event->masks = 0;
        ll_unlock_thread(thread);
        if (msg->data.us == 0) {
          ccsqueue_push(&freemq, &msg->node);
          continue;
        }
      }

#if 0
      if (robot->flag & ROBOT_YIELDABLE) {
        if (robot->co == 0) {
          robot->co = ll_new_coroutine(thread, robot);
        }
        if (robot->co->L != thread->L) {
          ccloge("wrong lua state");
          ccsqueue_push(&freemq, &msg->node);
          continue;
        }
        robot->co->bot = robot;
        robot->co->msg = msg;
        ccstate_resume(robot->co);
      } else {

      }
#endif

      robot->entry(robot, msg);

      /* robot run completed */
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

      /* currently the finished robot is removed out from thread's
      robot queue, but the robot still pointer to this thread.
      send SVDONE message to master to reset and free this robot. */
      ll_robot_send_message_sp(robot, ROBOT_ID_MASTER, MESSAGE_DELROBOT, robot);
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

