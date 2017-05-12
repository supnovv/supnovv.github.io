#include <stddef.h>
#include "service.h"
#include "luacapi.h"
#include "ionotify.h"

/**
 * threads container
 */

struct llthreads {
  struct ccpriorq priq;
  umedit_int seed;
};

static struct llthreads* ll_ccg_threads();
static nauty_bool ll_thread_less(void* elem, void* elem2);

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
  ccpriorq_push(&self->priq, &(thread->node));
  return thread;
}

/* no protacted, can only be called by master */
static void ll_thread_finish_a_service(struct ccthread* thread) {
  struct llthreads* self = ll_ccg_threads();
  if (thread->weight > 0) {
    thread->weight -= 1;
    ccpriorq_remove(&self->priq, &thread->node);
    ccpriorq_posh(&self->priq, &thread->node);
  }
}


/**
 * services container
 */

struct llservices {
  struct ccbackhash table;
  struct ccdqueue freeq;
  umedit_int freesize;
  umedit_int seed;
  struct ccmutex mutex;
};

#define LLSERVICE_START_SVID 0x2001
static struct llservices* ll_ccg_services();
static void ccservice_init(struct ccservice* self);
static umedit_int ll_service_getkey(void* service);
static int ll_service_tlink_offset();

static void llservices_init(struct llservices* self, nauty_byte initsizebits) {
  ccbackhash_init(&self->table, initsizebits, ll_service_tlink_offset, ll_service_getkey);
  ccdqueue_init(&self->freeq);
  ccmutex_init(&self->mutex);
  self->freesize = 0;
  self->seed = LLSERVICE_START_SVID;
}

static void llservices_free(struct llservice* self) {
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

static struct ccservice* ll_find_service(umedit_int svid) {
  struct llservices* self = ll_ccg_services();
  struct ccservice* srvc = 0;

  ccmutex_lock(&self->mutex);
  srvc = (struct ccservice*)ccbackhash_find(&self->table, svid);
  ccmutex_unlock(&self->mutex);

  return srvc;
}

static void ll_add_new_service_to_table(struct ccservice* srvc) {
  struct llservices* self = ll_ccg_services();
  ccmutex_lock(&self->mutex);
  ccbackhash_add(&self->table, srvc);
  ccmutex_unlock(&self->mutex);
}


/**
 * messages container
 */

struct llmessages {
  struct ccsqueue rxq;
  struct ccsqueue freeq;
  umedit_int freesize;
  struct ccmutex mutex;
};

static struct llmessages* ll_ccg_messages();

static void llmessages_init(struct llmessages* self) {
  ccsqueue_init(&self->rxq);
  ccsqueue_init(&self->freeq);
  ccmutex_init(&self->mutex);
  self->freesize = 0;
}

static void llmessages_free(struct llmessages* self) {
  ccmutex_free(&self->mutex);
}

static void ll_wakeup_master() {
  struct ccionfmgr* ionf = ccgetionfmgr();
  ccionfmgr_wakeup(ionf);
}

static void ll_send_message(struct ccmsgnode* msg) {
  struct llmessages* self = ll_ccg_messages();

  /* push the message */
  ccmutex_lock(&self->mutex);
  ccsqueue_push(&self->rxq, &msg->node);
  ccmutex_unlock(&self->mutex);

  /* wakeup master to handle the message */
  ll_wakeup_master();
}

static struct ccmsgnode* ll_new_msg() {
  struct llmessages* self = ll_ccg_messages();
  struct ccmsgnode* msg = 0;

  ccmutex_lock(&self->mutex);
  msg = (struct ccmsgnode*)ccsqueue_pop(&self->freeq);
  if (self->freesize > 0) {
    self->freesize -= 1;
  }
  ccmutex_unlock(&self->mutex);

  if (msg == 0) {
    msg = (struct ccmsgnode*)ccrawalloc(sizeof(struct ccmsgnode));
  }
  msg->node.next = msg->extra = 0;
  return msg;
}


/**
 * global structure
 */

struct ccglobal {
  struct llthreads threads;
  struct llservices services;
  struct llmessages messages;
  struct ccionfmgr ionf;
  struct ccthrkey thrkey;
  struct ccthread* mainthread;
};

static struct ccglobal* ccG;

static struct llthreads* ll_ccg_threads() {
  return &(ccG->threads);
}

static struct llservices* ll_ccg_services() {
  return &(ccG->services);
}

static struct llmessages* ll_ccg_messages() {
  return &(ccG->messages);
}

static nauty_bool ll_set_thread_data(struct ccthread* thread) {
  return ccthrkey_setdata(&ccG->thrkey, thread);
}

static void ccglobal_init(struct ccglobal* self, struct ccthread* master) {
  cczeron(self, sizeof(struct ccglobal));
  llthreads_init(&self->threads);
  llservices_init(&self->services);
  llmessages_init(&self->messages);

  ccionfmgr_init(&self->ionf);
  ccthrkey_init(&self->thrkey);

  ccG = self;
  self->mainthread = master;
  ll_set_thread_data(master);
}

static void ccglobal_free(struct ccglobal* self) {
  llthreads_free(&self->threads);
  llservices_free(&self->services);
  llmessages_free(&self->messages);

  ccionfmgr_free(&self->ionf);
  ccthrkey_free(&self->thrkey);
}

struct ccionfmgr* ccgetionfmgr() {
  return &(ccG->ionf);
}


/**
 * thread structure
 */

struct ccthread {
  /* access by master only */
  struct cclinknode node;
  umedit_int weight;
  /* shared with master */
  struct ccdqueue srvcrxq;
  struct ccmutex mutex;
  struct cccondv condv;
  /* free access */
  umedit_int index;
  struct ccthrid id;
  /* thread own use */
  struct lua_State* L;
  int (*start)();
  struct ccdqueue srvcq;
  struct ccsqueue msgq;
  struct ccsqueue freeco;
  struct ccstring defstr;
};

static nauty_bool ll_thread_less(void* elem, void* elem2) {
  struct ccthread* thread = (struct ccthread*)elem;
  struct ccthread* thread2 = (struct ccthread*)elem2;
  return thread->weight < thread2->weight;
}

static void ccthread_free(struct ccthread* self) {
  self->start = 0;
  if (self->L) {
   cclua_close(self->L);
   self->L = 0;
  }
  ccstrfree(&self->defstr);
}

void ccthread_init(struct ccthread* self) {
  cczeron(self, sizeof(struct ccthread));
  cclinknode_init(&self->node);
  ccdqueue_init(&self->srvcrxq);
  ccdqueue_init(&self->srvcq);
  ccsqueue_init(&self->msgq);
  ccsqueue_init(&self->freeco);

  ccmutex_init(&self->mutex);
  cccondv_init(&self->condv);

  self->L = cclua_newstate();
  self->defstr = ccemptystr();
  self->index = ll_new_thread_index();
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

static void ll_lock_thread(struct ccthread* self) {
  ccmutex_lock(&self->mutex);
}

static void ll_unlock_thread(struct ccthread* self) {
  ccmutex_unlock(&self->mutex);
}

static void ll_parse_command_line(int argc, char** argv) {
  (void)argc;
  (void)argv;
}

int startmainthreadcv(int (*start)(), int argc, char** argv) {
  int n = 0;
  struct ccglobal G;
  struct ccthread master;

  ccthread_init(&master);
  master.id = ccplat_selfthread();

  ccglobal_init(&G, &master);
  ll_parse_command_line(argc, argv);

  n = start();
  ccglobal_free(&G);
  ccthread_free(&master);
  return n;
}

int startmainthread(int (*start)()) {
  return startmainthreadcv(start, 0, 0);
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
 * service structure
 */

#define CCSERVICE_DONE 0x01
#define CCSERVICE_FUNC 0x01
#define CCSERVICE_CCCO 0x02
#define CCSERVICE_LUAF 0x03

struct ccservice {
  /* shared with master */
  struct cclinknode node;
　　struct ccsmplnode tlink;
  struct ccthread* thrd; /* modify by master only */
  struct ccsqueue rxmq;
  ushort_int outflag;
  /* thread own use */
  umedit_int svid;
  ushort_int flag;
  void* udata;
  struct ccluaco* co;
  union {
  int (*func)(void* udata, void* msg);
  int (*cofunc)(struct ccluaco*);
  } u;
};

static void ccservice_init(struct ccservice* self) {
  cczeron(self, sizeof(struct ccservice));
  cclinknode_init(&self->node);
  ccsmplnode_init(&self->tlink);
  ccsqueue_init(&self->rxmq);
}

static umedit_int ll_service_getkey(void* service) {
  return ((struct ccservice*)service)->svid;
}

static int ll_service_tlink_offset() {
  return offsetof(struct ccservice, tlink);
}

static struct ccservice* ll_new_service(umedit_int svid) {
  struct llservices* self = ll_ccg_services();
  struct ccservice* srvc = 0;
  nauty_byte newcreated = false;

  ccmutex_lock(&self->mutex);
  srvc = (struct ccservice*)ccdqueue_pop(&self->freeq);
  if (self->freesize > 0) {
    self->freesize -= 1;
  }
  ccmutex_unlock(&self->mutex);

  if (srvc == 0) {
    srvc = (struct ccservice*)ccrawalloc(sizeof(struct ccservice));
    newcreated = true;
  }

  ccservice_init(srvc);
  if (svid == 0) {
    srvc->svid = ll_new_service_id();
  } else {
    srvc->svid = svid;
  }

  if (newcreated) {
    ll_add_new_service_to_table(srvc);
  }
  return srvc;
}

struct ccservice* ccservice_new() {
  return ll_new_service(0);
}

void ccservice_sendmsg(struct ccservice* self, umedit_int destsvid, uright_int data, void* ptr) {
  struct ccmsgnode* msg = ll_new_msg();
  msg->srcid = self->svid;
  msg->dstid = destsvid;
  msg->data = data;
  msg->ptr = ptr;
  llservice_sendmsg(msg);
}

void ccservice_sendtomaster(struct ccservice* self, uright_int data) {
  struct ccmsgnode* msg = ll_new_msg();
  msg->srcid = self->svid;
  msg->dstid = 0;
  msg->data = data;
  msg->ptr = 0;
  llservice_sendmsg(msg);
}

void ccmaster_handlemsg(struct ccmsgnode* msg) {
  (void)msg;
}

void ccmaster_dispatch() {
  struct ccsqueue queue;
  struct ccsqueue freeq;
  struct ccmsghead* msg;
  struct ccservice* sv;
  struct ccthread* master = ccthead_getmaster();

  /* get all global messages */
  llthread_lock(master);
  queue = *llglobalmq();
  ccsqueue_init(llglobalmq());
  llthread_unlock(master);

  /* prepare master's messages */
  while ((msg = (struct ccmsghead*)ccsqueue_pop(&queue))) {
    if (llismastermsg(msg)) {
      ccsqueue_push(&master->msgq, &msg->node);
    }
  }

  /* handle master's messages */
  ccsqueue_init(&freeq);
  while ((msg = (struct ccmsghead*)ccsqueue_pop(&master->msgq))) {
    ccmaster_handlemsg(msg);
    ccsqueue_push(&freeq, &msg->node);
  }

  /* dispatch service's messages */
  while ((msg = (struct ccmsghead*)ccsqueue_pop(&queue))) {
    nauty_bool newattach = false;

    /* get message's dest service */
    sv = lldestservice(msg);
    if (sv == 0) {
      ccsqueue_push(&freeq, &msg->node);
      continue;
    }

    if (sv->thrd) {
      nauty_bool done = false;
      llthread_lock(sv->thrd);
      done = (sv->oflag & CCSERVICE_DONE) != 0;
      llthread_unlock(sv->thrd);
      if (done) {
        sv->thrd = 0;
        ccsqueue_push(&freeq, &msg->node);
        /* TODO: reset and remove servie to free list */
        continue;
      }
    } else {
      if (sv->oflag & CCSERVICE_DONE) {
        ccsqueue_push(&freeq, &msg->node);
        /* TODO: reset and remove servie to free list */
        continue;
      }
      /* attach a thread to handle */
      sv->thrd = llgetidlethread();
      if (sv->thrd == 0) {
        /* TODO: thread busy, need pending */
      }
      newattach = true;
    }

    llthread_lock(sv->thrd);
    msg->extra = sv;
    ccsqueue_push(&sv->rxmq, &msg->node);
    if (newattach) {
      ccdqueue_push(&sv->thrd->workrxq, &sv->node);
    }
    llthread_unlock(sv->thrd);
  }

  if (!ccsqueue_isempty(&freeq)) {
    llthread_lock(master);
    /* TODO: add free msg to global free queue */
    llthread_unlock(master);
  }
}

void ccthread_dispatch(struct ccthread* thrd) {
  struct ccdqueue doneq;
  struct ccsqueue freemq;
  struct ccservice* sv;
  struct cclinknode* head;
  struct cclinknode* node;
  struct ccmsghead* msg;
  int n = 0;

  llthread_lock(thrd);
  ccdqueue_pushqueue(&thrd->workq, &thrd->workrxq);
  llthread_unlock(thrd);

  if (ccdqueue_isempty(&thrd->workq)) {
    /* no services need to run */
    return;
  }

  head = &(thrd->workq.head);
  node = head->next;
  for (; node != head; node = node->next) {
    sv = (struct ccservice*)node;
    if (!ccsqueue_isempty(&sv->rxmq)) {
      llthread_lock(thrd);
      ccsqueue_pushqueue(&thrd->msgq, &sv->rxmq);
      llthread_unlock(thrd);
    }
  }

  if (ccsqueue_isempty(&thrd->msgq)) {
    /* current services have no messages */
    return;
  }

  ccdqueue_init(&doneq);
  ccsqueue_init(&freemq);
  while ((msg = (struct ccmsghead*)ccsqueue_pop(&thrd->msgq))) {
    sv = (struct ccservice*)msg->extra;
    if (sv->iflag & CCSERVICE_DONE) {
      ccsqueue_push(&freemq, &msg->node);
      continue;
    }

    if (sv->flag & CCSERVICE_CCCO) {
      if (sv->co == 0) {
        if ((sv->co = (struct ccluaco*)ccsqueue_pop(&thrd->freeco)) == 0) {
          /* TODO: how to allocate ccluaco */
          /* sv->co = ccluaco_create(thrd, sv->u.cofunc, sv->udata); */
        }
      }
      if (sv->co->owner != thrd) {
        ccloge("runservice invalide luaco");
      }
      sv->co->msg = msg;
      n = ccluaco_resume(sv->co);
      sv->co->msg = 0;
    } else if (sv->flag & CCSERVICE_FUNC) {
      n = sv->u.func(sv->udata, &msg->node);
    } else if (sv->flag & CCSERVICE_LUAF) {

    }
    else {
      ccloge("run service invalide type");
    }

    /* service run completed */
    if (n == 0) {
      sv->iflag |= CCSERVICE_DONE;
      ccdqueue_push(&doneq, cclinknode_remove(&sv->node));
      if (sv->co) {
        ccsqueue_push(&thrd->freeco, &sv->co->node);
        sv->co = 0;
      }
    }
    /* free handled message */
    ccsqueue_push(&freemq, &msg->node);
  }

  if (!ccdqueue_isempty(&doneq)) {
    llthread_lock(thrd);
    while ((sv = (struct ccservice*)ccdqueue_pop(&doneq))) {
      sv->iflag &= (~CCSERVICE_DONE);
      sv->oflag |= CCSERVICE_DONE;
    }
    llthread_unlock(thrd);
  }

  if (!ccsqueue_isempty(&freemq)) {
    struct ccthread* master = ccthread_getmaster();
    llthread_lock(master);
    /* TODO: add free msg to global free queue */
    llthread_unlock(master);
  }
}

