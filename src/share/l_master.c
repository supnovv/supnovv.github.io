
static l_ionfmgr llg_ionf_mgr;
static l_thread* llg_master_thread;
static l_squeue llg_msg_rxq;
static l_mutex llg_msg_lock;

#define L_MIN_SERVICE_ID (1024*1024)

#define L_MESSAGE_START_SERVICE 0x01
#define L_MESSAGE_STOP_SERVICE 0x02
#define L_MESSAGE_REMOVE_EVENT 0x03

l_mutex llg_srvc_mtx;
l_umedit llg_svid_seed = L_MIN_SERVICE_ID;

static l_umedit ll_gen_service_id() {
  l_mutex* mtx = &llg_srvc_mtx;
  l_umedit svid = 0;

  l_mutex_lock(mtx);
  svid = ++llg_svid_seed;
  if (svid < L_MIN_SERVICE_ID) {
    svid = llg_svid_seed = L_MIN_SERVICE_ID;
  }
  l_mutex_unlock(mtx);

  return svid;
}

typedef struct {
  l_smplnode node;
} l_srvcslot;

typedef struct {
  l_umedit nslot;
  l_umedit nelem;
  l_srvcslot* slot;
} l_srvctable;

void l_srvctable_init(l_srvctable* self, l_byte sizebits) {
  if (sizebits > 30) {
    self->slot = 0;
    self->nelem = 0;
    l_loge_1("too large %d", ld(sizebits));
    return;
  }
  self->nslot = (1 << sizebits);
  self->slot = (l_srvcslot*)l_raw_calloc(sizeof(l_srvcslot)*self->nslot);
  self->nelem = 0;
}

void l_srvctable_free(l_srvctable* self) {
  if (self->slot) {
    l_raw_free(self->slot);
    self->slot = 0;
    self->nelem = 0;
  }
}

void l_srvctable_add(l_srvctable* self, l_service* elem) {
  l_srvcslot* slot = 0;
  if (elem == 0) return;
  slot = self->slot + (ll_make_hash(elem->fd) & (self->nslot - 1));
  elem->tnext = slot->tnext;
  slot->tnext = elem;
  self->nelem += 1;
  if (self->nelem > self->nslot) {
    l_logw_2("nelem %d nslot %d", ld(self->nelem), ld(self->nslot));
  }
}

void l_srvctable_foreach(l_srvctable* self, void (*cb)(l_service*)) {
  l_srvcslot* slot = self->slot;
  l_srvcslot* end = slot + self->nslot;
  l_service* elem = 0;
  if (self->slot == 0) return;
  for (; slot < end; ++slot) {
    elem = slot->tnext;
    while (elem) {
      cb(elem);
      elem = elem->tnext;
    }
  }
}

l_service* l_srvctable_find(l_srvctable* self, l_umedit svid) {
  l_srvcslot* slot = 0;
  l_service* ioev = 0;
  slot = self->slot + (ll_make_hash(fd) & (self->nslot - 1));
  ioev = slot->tnext;
  while (ioev) {
    if (ioev->fd == fd) return ioev;
    ioev = ioev->tnext;
  }
  return 0;
}

l_service* l_srvctable_del(l_srvctable* self, l_umedit svid) {
  l_srvcslot* slot = 0;
  l_service* elem = 0;
  l_service* next = 0;
  slot = self->slot + (ll_make_hash(fd) & (self->nslot - 1));
  elem = slot->tnext;
  if (!elem) return 0;
  if (elem->fd == fd) {
    slot->tnext = elem->tnext;
    slot->nelem -= 1;
    return elem;
  }
  while (elem->tnext) {
    next = elem->tnext;
    if (next->fd == fd) {
      elem->tnext = next->tnext;
      self->nelem -= 1;
      return next;
    }
    elem = next;
  }
  return 0;
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

void l_globalmsgq_create() {
  l_squeue_init(&llg_msg_rxq);
  l_mutex_init(&llg_msg_lock);
}

void l_globalmsgq_destroy() {
  l_message* msg = 0;
  while ((msg = (l_message*)l_squeue_pop(&llg_msg_rxq))) {
    l_raw_free(msg);
  }
  l_mutex_free(&llg_msg_lock);
}

l_squeue l_global_messages() {
  l_squeue q;
  l_squeue* mq = &llg_msg_rxq;
  l_mutex* mtx = &llg_msg_lock;

  l_mutex_lock(mtx);
  q = *mq;
  l_squeue_init(mq);
  l_mutex_lock(mtx);

  return q;
}

void llsendmessages(l_thread* thread) {
  l_mutex* mtx = &llg_msg_lock;

  l_mutex_lock(mtx);
  l_squeue_push_queue(&llg_msg_rxq, &thread->txmq);
  l_mutex_unlock(mtx);

  /* wakeup master to handle the message */
  l_ionfmgr_wakeup(ionf);
}

static l_service* llfindservice(l_umedit svid) {

}

static void ll_master_send_connrsp_message(l_umedit dstid, l_event* ev) {
  l_thread* master = l_master_thread();
  l_message* msg = l_create_message(master, L_MESSAGE_CONNRSP, sizeof(l_ioevent_message);
  l_connrsp_message* p = (l_connrsp_message*)msg;
  p->sock = ev->fd;
  p->masks = ev->masks;
  l_send_message(master, dstid, msg);
}

static void ll_accept_connection(void* ud, l_sockconn* conn) {
  l_service* sv = (l_service*)ud;
  l_thread* master = l_master_thread();
  l_message* msg = l_create_message(master, L_MESSAGE_CONNIND, sizeof(l_connind_message));
  l_connind_message* p = (l_connind_message*)msg;
  p->sock = conn->sock;
  p->remote = conn->remote;
  l_send_message(master, sv->svid, msg);
}

static void ll_master_dispatch_ioevent(l_ioevent* ev) {
  l_service* srvc = 0;
  l_ioevent* svio = 0;
  l_thread* thread = 0;
  l_mutex* emtx = 0;
  l_umedit type = 0;
  int unchained = false;

  srvc = ll_master_find_service(ev->svid);
  if (!srvc) {
    l_loge_s("service not found");
    return;
  }

  thread = srvc->belong;
  if (!thread) {
    l_loge_s("service not run");
    return;
  }

  svio = &srvc->io;
  if (svio->fd != ev->fd) {
    l_loge_s("io event mismatch");
    return;
  }

  emtx = thread->svmtx;
  l_mutex_lock(emtx);
  if (svio->falgs & L_EVENT_FLAG_LISTEN) {
      type = L_MESSAGE_CONNIND;
  } else if (svio->flags & L_EVENT_FLAG_CONNECT) {
      svio->flags &= (~L_EVENT_FLAG_CONNECT);
      type = L_MESSAGE_CONNRSP;
  } else {
    l_thread* master = l_master_thread();
    type = L_MESSAGE_IOEVENT;
    svio->masks |= ev->masks;
    if (!svio->chained) {
      svio->chained = srvc;
      unchained = true;
    }
  }
  l_mutex_unlock(emtx);

  if (unchained) {
    l_squeue_push(&master->rxio, &svio->node);
  }

  if (type == L_MESSAGE_CONNIND) {
    l_socket_accept(sv->event->fd, ll_accept_connection, sv);
  } else if (type == L_MESSAGE_CONNRSP) {
    ll_master_send_connrsp_message(srvc->dstid, ev);
  }
}

static void ll_master_handle_message(l_squeue* frmq) {
  l_thread* master = l_master_thread();
  l_message* msg = 0;
  l_squeue msgq;

  l_squeue_init(&msgq);
  l_thread_lock(master);
  l_squeue_push_queue(&msgq, &master->rxmq);
  l_thread_unlock(master);

  while ((msg = (l_message*)l_squeue_pop(msgq))) {
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
}

static int ll_check_service_completed(l_service* srvc, l_umedit destid) {
  if (sv->belong) {
        nauty_bool done = false;

        ll_lock_thread(sv->belong);
        done = (sv->outflag & L_SERVICE_RUNNING) == 0;
        ll_unlock_thread(sv->belong);

        if (done || srvc->svid != destid) {
          /* free current msg and service's received msgs (if any) */
          l_squeue_push(&freeq, &msg->node);
          l_squeue_pushqueue(&freeq, &sv->rxmq);

          /* when service is done, the service is already removed out from thread's service queue.
          here delete service from the hash table and insert into service's free queue */
          ll_service_is_finished(sv);

          /* reset the service */
          sv->belong = 0;
          return true;
        }
      } else {
        /* attach a thread to handle the service */
        sv->belong = ll_get_thread_for_new_service();
        newattach = true;
      }
      return false;
}

void l_master_start() {
  l_squeue rxmq, frmq;
  l_message* msg = 0;
  l_service* srvc = 0;
  l_thread* thread = 0;
  l_thread* master = l_master_thread();
  l_ioevent* ioev = 0;
  l_mutex* emtx = 0;

  l_squeue_init(&rxmq);
  l_squeue_init(&frmq);

  for (; ;) {
    l_ionfmgr_wait(ll_get_ionfmgr(), l_master_dispatch_event);

    /* handle master's messages */

    ll_master_handle_message(&frmq);

    /* dispatch service's messages */
    ll_get_current_msgs(&rxmq);
    l_squeue_push_queue(&rxmq, &master->txmq);
    while ((msg = (l_message*)l_squeue_pop(&rxmq))) {

      /* get message's dest service */
      srvc = ll_find_service(msg->dstid);
      if (srvc == 0) {
        l_loge_1("service not found %d", ld(msg->dstid));
        l_squeue_push(&frmq, &msg->node);
        continue;
      }

      if ((ll_check_service_completed(srvc, msg->dstid)) {
        continue;
      }

      thread = srvc->belong;
      msg->extra = srvc;
      l_thread_lock(thread);
      l_squeue_push(&thread->rxmq, &msg->node);
      if (!thread->msgwait) {
        thread->msgwait = 1;
        l_condv_signal(&thread->condv);
      }
      l_thread_unlock(thread);
    }

    l_free_messages(master, &frmq);

    /* dispatch io event */
    while ((ioev = (l_ioevent*)l_squeue_pop(&master->rxio))) {
      srvc = ioev->chained;
      thread = srvc->belong;

      if ((ll_check_service_completed(service, ioev->svid)) {
        ioev->chained = 0;
        ioev->masks = 0;
        continue;
      }

      emtx = thread->elock;
      l_mutex_lock(emtx);
      l_squeue_push(&thread->rxio, &ioev->node);
      l_mutex_unlock(emtx);

      l_thread_lock(thread);
      if (!thread->iowait) {
        thread->iowait = 1;
        l_condv_signal(&thread->condv);
      }
      l_thread_unlock(thread);
    }
  }
}

static void ll_deliver_service_msg(l_service* srvc, l_message* msg) {
  l_thread* thread = srvc->belong;

  srvc->entry(srvc, msg);
  if (srvc->flag & L_SERVICE_RUNNING) return;

  if (srvc->co) {
    ll_free_coroutine(thread, service->co);
    service->co = 0;
  }

  ll_lock_thread(thread);
  service->outflag &= (~L_SERVICE_RUNNING);
  ll_unlock_thread(thread);

  /* currently the finished service is removed out from thread's
  service queue, but the service still pointer to this thread.
  send SVDONE message to master to reset and free this service. */
  ll_service_send_message_sp(service, L_SERVICE_MASTERID, L_MESSAGE_DELSERVICE, service);
}

static void ll_thread_flush_messages(l_thread* self) {
  l_message* msg = 0;
  l_thread* master = l_master_thread();
  l_mutex* mtx = &llg_msg_lock;
  l_squeue* txmq = &self->txmq;
  l_squeue* txms = &self->txms;
  int sent = false;

  if (!l_squeue_is_empty(txms)) {
    l_thread* master = l_master_thread();
    l_thread_lock(master);
    l_squeue_push_queue(&master->rxmq, txms);
    l_thread_unlock(master);
    sent = true;
  }

  if (!l_squeue_is_empty(txmq)) {
    l_mutex_lock(mtx);
    l_squeue_push_queue(&llg_msg_rxq, txmq);
    l_mutex_unlock(mtx);
    sent = true;
  }

  if (sent) {
    ll_wakeup_master();
  }
}

/* SERVICE要么保存在hashtable中，要么保存在free队列中，
一个SERVICE只分配给一个线程处理，当某个SERVICE的消息到来
时, 从消息可以获取SERVICE号，由SERVICE号从hashtable中
取得service对象，这个service对象会保存到消息extra中，然
后将消息插入线程的消息队列中等待线程处理。当线程判断有消息时，
依次取出进行处理，

1. Service监听一个sock时，新建一个l_ioevent发送给master添加到epoll中，发送前将这个ioevent添加到对应线程hash表中
2. master添加成功或失败，都将l_ioevent添加到自己的free队列（不需要释放，这个ioevent始终在hash表中）
3. 等待的io事件到来后，会回调master的函数
4. master检查这个l_ioevent是否已经在对应的线程等待hash表中
5. 如果在直接更新masks即可，否则新建一个l_ioevent（如果没有指定为0）发消息给线程表示该sock有事件发生（不需要发消息，将iovent插入线程的rxio队列即可）
6. 线程处理io事件时，一个io事件始终只要一个msg即可

MASTER检查时，如果masks不为0表示线程还为处理，简单更新masks即可
如果为0，表示这个是第一个事件或线程已经处理完了，则将这个io插入到线程的rxio队列中

方案二，只让Service对应接收一个io event的通知处理：
1. 添加event时，所在线程分配一个l_ioevent发送给master创建SERVICE，这个l_ioevent会挂在SERVICE上
2. master收到事件通知时，查看service上挂的l_ioevent，如果masks为0则将l_ioevent添加到线程rxio队列，否则直接更新masks即可
3. 线程处理通知，sock关闭时，将服务上挂载的对应l_ioevent（如果没有chained）释放到自己的free buffer pool，并将sock发给master停止监听并让master关闭sock
4. 工作线程中的服务工作完成时，如果没有chained释放到free buffer pool，如果有chained先断开与service的联系，然后在处理该l_ioevent时释放

***需要标记service能不能释放***
*/

void l_worker_start() {
  l_squeue msgq, frmq, rxio;
  l_service* service = 0;
  l_message* msg = 0;
  l_thread* thread = l_thread_self();
  l_mutex* emtx = 0;
  l_ioevent* ioev = 0;
  l_ioevent_message iomsg;

  l_squeue_init(&msgq);
  l_squeue_init(&rxio);
  l_squeue_init(&frmq);

  for (; ;) {
    l_thread_lock(thread);
    while (l_squeue_is_empty(&thread->rxmq) && l_squeue_is_empty(&thread->rxio)) {
      thread->msgwait = thread->iowait = 0;
      l_condv_wait(&thread->condv, &thread->mutex);
    }
    l_squeue_push_queue(&msgq, &thread->rxmq);
    l_squeue_push_queue(&rxio, &thread->rxio);
    l_thread_unlock(thread);

    while ((msg = (l_message*)l_squeue_pop(&msgq))) {
      service = (l_service*)msg->extra;
      if (!(service->flag & L_SERVICE_RUNNING) || msg->svid != service->svid) {
        /* completed or the service is released or re-newed (new svid) */
        l_squeue_push(&frmq, &msg->node);
        continue;
      }

      if (msg->svid != service->svid) {

      }

      ll_deliver_service_msg(service, msg);

      /* free handled message */
      l_squeue_push(&freemq, &msg->node);
    }

    emtx = thread->elock;

    while ((ioev = (l_iovent*)l_squeue_pop(&rxio)) {
      /* event's content except 'node' field is guarded by elock */
      l_mutex_lock(emtx);
      ev = *ioev;
      ioev->masks = 0;
      ioev->chained = 0;
      l_mutex_unlock(emtx);

      service = ev.chained;

      if (!(service->flag & L_SERVICE_RUNNING) || msg->svid != service->svid) {
        /* completed or the service is released or re-newed (new svid) */
        l_squeue_push(&frmq, &msg->node);
        continue;
      }

      iomsg.extra = service;
      iomsg.dstid = service.dstid;
      iomsg.type = L_MESSAGE_IOEVENT;
      iomsg.sock = ev.fd;
      iomsg.masks = ev.masks;
      service->entry(service, &iomsg);

      ll_deliver_service_msg(service, msg);
    }

    l_free_messages(thread, &frmq);

    ll_thread_flush_messages(thread);
  }
}

static void llparsecommandline(int argc, char** argv) {
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
  llparsecommandline(argc, argv);
  n = start();

  /* clean */
  l_thread_free(&master);
  l_global_free(&G);
  return n;
}

int l_master_start(int (*start)(), int argc, char** argv) {
  return startmainthreadcv(start, argc, argv);
}

#include "service.h"

struct ccionfslot {
  struct ccsmplnode head;
};

struct ccionfnode {
  struct ccionfnode* bucket_next_dont_use_;
  struct ccionfnode* qnext; /* union with size/type/flag in ccmsghead */
};

struct ccpqueue {
  struct ccionfnode head;
  struct ccionfnode* tail;
};

struct ccionfpool {
  umedit_int nslot; /* prime number size not near 2^n */
  umedit_int nfreed, nbucket, qsize;
  struct ccpqueue queue; /* chain all ccionfmsg fifo */
  struct ccsmplnode freelist;
  struct ccionfslot slot[1];
};

struct ccionfmgr {
  struct ccionfpool* pool;
};

struct ccionfevt;
struct ccionfmsg;

nauty_bool ccionfevt_isempty(struct ccionfevt* self);
void ccionfevt_setempty(struct ccionfevt* self);
struct ccionfmsg* ccionfmsg_new(struct ccionfevt* event);
struct ccionfmsg* ccionfmsg_set(struct ccionfmsg* self, struct ccionfevt* event);

void ccpqueue_init(struct ccpqueue* self) {
  self->head.qnext = self->tail = &self->head;
}

void ccpqueue_push(struct ccpqueue* self, struct ccionfnode* newnode) {
  newnode->qnext = self->tail->qnext;
  self->tail->qnext = newnode;
  self->tail = newnode;
}

nauty_bool ccpqueue_isempty(struct ccpqueue* self) {
  return (self->head.qnext == &self->head);
}

struct ccionfnode* ccpqueue_pop(struct ccpqueue* self) {
  struct ccionfnode* node = 0;
  if (ccpqueue_isempty(self)) {
    return 0;
  }
  node = self->head.qnext;
  self->head.qnext = node->qnext;
  if (self->tail == node) {
    self->tail = &self->head;
  }
  return node;
}

#define CCSOCK_ACCEPT 0x01
#define CCSOCK_CONNET 0x02

#define CCMSGTYPE_SOCKMSG 1
#define CCMSGFLAG_FREEMEM 0x0001
#define CCMSGFLAG_NEWCONN 0x0100
#define CCMSGFLAG_CONNEST 0x0200

struct ccsockmsg* ccnewsockmsg(int sockfd, umedit_int events, void* ud) {
  struct ccsockmsg* sm = (struct ccsockmsg*)ccrawalloc(sizeof(struct ccsockmsg));
  sm->head.size = sizeof(struct ccsockmsg);
  sm->head.type = CCMSGTYPE_SOCKMSG;
  sm->head.flag = CCMSGFLAG_FREEMEM;
  sm->head.data = ud;
  sm->sockfd = sockfd;
  sm->events = events;
  return sm;
}

void ccsocknewconn(struct ccepoll* self, int connfd, struct ccwork* work) {
  struct ccsockmsg* sm = ccnewsockmsg(connfd, 0, ud);
  sm->head.flag |= CCMSGFLAG_NEWCONN;
}

void ccepolldispatch(struct ccepoll* self) {
  int i = 0, n = self->n;
  struct epoll_event* ready = self->ready;
  struct ccevent* e = 0;
  for (; i < n; ++i) {
    e = (struct ccevent*)ready[i].data.ptr;
    e->revents = (umedit_int)ready[i].events;
    if ((e->revents | CCEPOLLIN) && (e->waitop & CCSOCK_ACCEPT)) {
      ccsockaccept(self, e);
    }
  }
}

struct ccionf {
  int epfd;
  int n, maxlen;
  struct epoll_event ready[CCIONF_MAX_WAIT_EVENTS];
};

struct ccionfevt {
  int fd;
  umedit_int events;
  void* ud;
};

struct ccionfmsg {
  struct ccmsghead head;
  struct ccionfevt event;
};

nauty_bool ccionfevtvoid(struct ccionfevt* self) {
  return (self->fd == -1);
}

struct ccionfmsg* ccionfmsgnew(struct ccionfevt* event) {
  struct ccionfmsg* self = (struct ccionfmsg*)ccrawalloc(sizeof(struct ccionfmsg));
  return ccionfmsgset(self, event);
}

struct ccionfmsg* ccionfmsgset(struct ccionfmsg* self, struct ccionfevt* event) {
  self->event = *event;
  return self;
}


umedit_int llionfpool_hash(struct ccionftbl* self, umedit_int fd) {
  return fd % self->size; /* size should be prime number not near 2^n */
}

umedit_int llionfpool_size(uoctect_int bits) {
  /* cchashprime(bits) return a prime number < (1 << bits) */
  return cchashprime(bits);
}

void llionfpool_addtofreelist(struct ccionftbl* self, struct ccsmplnode* node) {
  ccsmplnode_insertafter(&self->freelist, node);
  self->nfreed += 1;
}

struct ccionfmsg* llionfpool_getfromfreelist(struct ccionftbl* self, struct ccionfevt* event) {
  struct ccionfmsg* p = 0;
  if (((p = struct ccionfmsg*)ccsmplnode_removenext(&self->freelist)) == 0) {
    return 0;
  }
  if (self->nfreed > 0) {
    self->nfreed -= 1;
  } else {
    ccloge("ccionfpool freelist size invalid");
  }
  return llionfmsg_set(p, event);
}

struct ccionfpool* ccionfpool_new(uoctect_int sizenumbits) {
  struct ccionfpool* pool = 0;
  sright_int size = llionfpool_size(sizenumbits);
  pool = (struct ccionfpool*)ccrawalloc(sizeof(struct ccionfpool) + size * sizeof(struct ccionfslot));
  pool->nslot = size;
  while (size > 0) {
    ccsmplnode_init(&(pool->slot[--size].head));
  }
  ccsmplnode_init(&pool->freelist);
  ccpqueue_init(&pool->queue);
  pool->nfreed = pool->nbucket = pool->qsize = 0;
  return pool;
}

struct ccsmplnode* ccionfpool_addevent(struct ccionfpool* self, struct ccionfevt* newevent) {
  struct ccionfmsg* msg = 0;
  struct ccionfslot* slot = 0;
  struct ccsmplnode* head = 0;
  struct ccsmplnode* cur = 0;
  if (llionfevt_isempty(newevent)) {
    ccloge("addEvent invalid event");
    return 0;
  }
  slot = self->slot + ccionfpool_hash(self, (umedit_int)newevent->fd);
  for (head = &slot->head, cur = head; cur->next != head; cur = cur->next) {
    msg = (struct ccionfmsg*)cur->next;
    /* if current event is new, than it will lead all empty nodes freed */
    if (llionfevt_isempty(&msg->event)) {
      llionfpool_addtofreelist(self, ccsmplnode_removenext(cur));
      if (self->nbucket > 0) {
        self->nbucket -= 1;
      } else {
        ccloge("ccionfpool bucket size invalid");
      }
      continue;
    }
    if (msg->event.fd == newevent->fd) {
      msg->event.events |= newevent->events;
      return 0;
    }
  }
  if ((msg = llionfpool_getfromfreelist(self, newevent)) == 0) {
    msg = ccepollmsg_new(newevent);
  }
  ccsmplnode_insertafter(&slot->node, (struct ccsmplnode*)msg);
  self->nbucket += 1;
  ccpqueue_push(&self->queue, msg);
  self->qsize += 1;
  return msg;
}

int ccnodemain(struct ccstate* s) {
  struct ccglobal G;
  ccinitglobal(&G, "ccnode.conf");
  struct ccstate s[G.workers+1];
  int i = 0;
  ccinitstatepool(&G, s+1, G.workers);
  for (; i < G.workers + 1; ++i) {

  }
  ccinitstate(&s[0], G, pthread_self());
  G.iofd = llepollcreate();
  if (G.iofd == -1) return -1;
}

int main(int argc, char** argv) {
  return startmainthreadcv(ccmaster_start, argc, argv);
}

