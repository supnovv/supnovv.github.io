#include "thatcore.h"
#include "l_thread.h"
#include "l_message.h"
#include "l_service.h"

l_thrkey llg_thread_key;
l_thread_local(l_thread* llg_thread_ptr);

static l_priorq llg_thread_prq;
static l_thread* llg_threads;
static int llg_num_threads;

static l_thread* llg_master_thread;
static l_ionfmgr llg_ionf_mgr;

static l_squeue llg_msg_rxq;
static l_mutex llg_msg_lock;

static l_mutex llg_srvc_mtx;
static l_umedit llg_svid_seed;
static l_srvctable llg_srvc_table;
static l_squeue llg_srvc_frq;

typedef struct {
  l_mutex ma;
  l_mutex mb;
  l_condv ca;
  l_logger l;
  l_freeq co;
  l_freeq bf;
  l_squeue qa;
  l_squeue qb;
  l_squeue qc;
  l_squeue qd;
} l_thrblock;

static int llthreadless(void* lhs, void* rhs) {
  return ((l_thread*)lhs)->weight < ((l_thread*)rhs)->weight;
}

void l_threadpool_create(int numofthread) {
  l_thread* t = 0;
  l_thrblock* b = 0;

  l_thrkey_init(&llg_thread_key);
  l_priorq_init(&llg_thread_prq, llthreadless);

  while (numofthread-- > 0) {
    t = (l_thread*)l_raw_calloc(sizeof(l_thread));
    t->block = l_raw_malloc(sizeof(l_thrblock));
    t->index = (l_ushort)numofthread;
    b = (l_thrblock*)t->block;
    t->svmtx = &b->ma;
    t->mutex = &b->mb;
    t->condv = &b->ca;
    t->log = &b->l;
    t->frco = &b->co;
    t->frbf = &b->bf;
    t->rxmq = &b->qa;
    t->rxio = &b->qb;
    t->txmq = &b->qc;
    t->txms = &b->qd;
    l_priorq_push(&llg_thread_prq, &t->node);
  }
}

void l_threadpool_destroy() {
  l_thread* t = 0;

  l_thrkey_free(&llg_thread_key);

  while ((t = (l_thread*)l_priorq_pop(&llg_thread_prq))) {
    l_raw_free(t->block);
    l_raw_free(t);
  }
}

l_thread* l_threadpool_acquire() {
  l_thread* thread = (l_thread*)l_priorq_pop(&llg_thread_prq);
  thread->weight += 1;
  l_priorq_push(&llg_thread_prq, &thread->node);
  return thread;
}

void l_threadpool_release(l_thread* thread) {
  thread->weight -= 1;
  l_priorq_remove(&llg_thread_prq, &thread->node);
  l_priorq_push(&llg_thread_prq, &thread->node);
}

typedef struct {
  l_smplnode node;
} l_srvcslot;

typedef struct {
  l_umedit nslot;
  l_umedit nelem;
  l_srvcslot* slot;
} l_srvctable;

static void l_srvctable_init(l_srvctable* self, l_byte sizebits) {
  if (sizebits > 30) {
    self->slot = 0;
    self->nelem = 0;
    l_loge_1("slots 2^%d", ld(sizebits));
    return;
  }
  self->nslot = (1 << sizebits);
  self->slot = (l_srvcslot*)l_raw_calloc(sizeof(l_srvcslot)*self->nslot);
  self->nelem = 0;
}

static l_smplnode* llheadnode(l_srvctable* self, l_umedit svid) {
  return &(self->slot[svid & (self->nslot - 1)].node);
}

static void l_srvctable_add(l_srvctable* self, l_smplnode* elem) {
  l_smplnode* slot = 0;
  if (elem == 0) return;
  slot = llheadnode(self, ((l_service*)elem)->svid);
  elem->next = slot->next;
  slot->next = elem;
  self->nelem += 1;
  if (self->nelem > self->nslot) {
    l_logw_2("nelem %d nslot %d", ld(self->nelem), ld(self->nslot));
  }
}

static l_service* l_srvctable_find(l_srvctable* self, l_umedit svid) {
  l_smplnode* slot = 0;
  l_smplnode* srvc = 0;
  slot = llheadnode(self, svid);
  srvc = slot->next;
  while (srvc) {
    if (((l_service*)srvc)->svid == svid) return (l_service*)srvc;
    srvc = srvc->next;
  }
  return 0;
}

static l_service* l_srvctable_del(l_srvctable* self, l_umedit svid) {
  l_smplnode* elem = 0;
  l_smplnode* node = 0;
  elem = llheadnode(self, svid);
  while (elem->next) {
    node = elem->next;
    if (((l_service*)node)->svid == svid) {
      elem->next = node->next;
      self->nelem -= 1;
      return (l_service*)node;
    }
    elem = node;
  }
  return 0;
}

static void l_srvctable_foreach(l_srvctable* self, void (*cb)(l_service*)) {
  l_srvcslot* slot = self->slot;
  l_srvcslot* end = slot + self->nslot;
  l_smplnode* elem = 0;
  if (self->slot == 0) return;
  for (; slot < end; ++slot) {
    elem = slot->node.next;
    while (elem) {
      if (cb) cb((l_service*)elem);
      elem = elem->next;
    }
  }
}

static void l_srvctable_clear(l_srvctable* self, void (*elemfree)(l_service*)) {
  l_srvcslot* slot = self->slot;
  l_srvcslot* end = slot + self->nslot;
  l_smplnode* first = 0;
  if (self->slot == 0) return;
  for (; slot < end; ++slot) {
    first = slot->node.next;
    while (first) {
      if (elemfree) elemfree((l_service*)first);
      slot->node.next = first->next;
    }
  }
}

static void l_srvctable_free(l_srvctable* self, void (*elemfree)(l_service*)) {
  if (!self->slot) return;
  ll_srvctable_clear(self, elemfree);
  l_raw_free(self->slot);
  self->slot = 0;
  self->nelem = 0;
}

#define L_MESSAGE_START_SERVICE 0x01
#define L_MESSAGE_CLOSE_SERVICE 0x02
#define L_MESSAGE_CLOSE_SRVCRSP 0x03
#define L_MESSAGE_CLOSE_EVENTFD 0x04
#define L_MIN_SERVICE_ID (1024*1024)

static l_master_init(l_thread* master, l_byte srvctablesizebits) {
  llg_master_thread = master;
  l_ionfmgr_init(&llg_ionf_mgr);

  l_squeue_init(&llg_msg_rxq);
  l_mutex_init(&llg_msg_lock);

  l_mutex_init(&llg_srvc_mtx);
  llg_svid_seed = L_MIN_SERVICE_ID;
  l_srvctable_init(&llg_srvc_table,
    srvctablesizebits < 10 ? 10 : srvctablesizebits);
  l_squeue_init(&llg_srvc_frq);
}

static l_master_free(void (*srvcfree)(l_service*)) {
  l_message* msg = 0;
  l_squeue* rxmq = &llg_msg_rxq;
  l_mutex* mtx = &llg_msg_lock;
  l_service* srvc = 0;
  l_squeue* svfrq = &llg_srvc_frq;

  l_ionfmgr_free(&llg_ionf_mgr);

  l_mutex_lock(mtx);
  while ((msg = (l_message*)l_squeue_pop(rxmq)) {
    l_raw_free(msg);
  }
  l_mutex_unlock(mtx);
  l_mutex_free(mtx);

  l_mutex_free(&llg_srvc_mtx);
  l_srvctable_free(&llg_srvc_table, srvcfree);

  while ((srvc = (l_service*)l_squeue_pop(svfrq) {
    l_raw_free(srvc);
  }
}

l_ionfmgr* l_master_get_ionfmgr() {
  return &llg_ionf_mgr;
}

l_thread* l_master_thread() {
  return llg_master_thread;
}

l_umedit l_master_gen_service_id() {
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

static void l_master_add_service(l_service* srvc) {
  l_srvctable_add(&llg_srvc_table, &srvc->node);
}

static l_service* l_master_find_service(l_umedit svid) {
  return l_srvctable_find(&llg_srvc_table, svid);
}

static l_service* l_master_del_service(l_umedit svid) {
  return l_srvctable_del(&llg_srvc_table, svid);
}

static void l_master_get_messages(l_squeue* outq) {
  l_squeue* mq = &llg_msg_rxq;
  l_mutex* mtx = &llg_msg_lock;

  l_mutex_lock(mtx);
  l_squeue_push_queue(outq, mq);
  l_mutex_lock(mtx);
}

static void l_master_wakeup() {
  /* wakeup master to handle the message */
  l_ionfmgr_wakeup(l_master_get_ionfmgr());
}

static void l_master_send_connrsp_message(l_umedit dstid, l_event* ev) {
  l_thread* master = l_master_thread();
  l_message* msg = l_create_message(master, L_MESSAGE_CONNRSP, sizeof(l_ioevent_message));
  l_connrsp_message* p = (l_connrsp_message*)msg;
  p->sock = ev->fd;
  p->masks = ev->masks;
  l_send_message(master, dstid, msg);
}

static void l_master_accept_connection(void* ud, l_sockconn* conn) {
  l_service* sv = (l_service*)ud;
  l_thread* master = l_master_thread();
  l_message* msg = l_create_message(master, L_MESSAGE_CONNIND, sizeof(l_connind_message));
  l_connind_message* p = (l_connind_message*)msg;
  p->sock = conn->sock;
  p->remote = conn->remote;
  l_send_message(master, sv->svid, msg);
}

static void l_master_dispatch_event(l_ioevent* rxev) {
  l_thread* master = l_master_thread();
  l_service* srvc = 0;
  l_thread* thread = 0;
  l_mutex* svmtx = 0;
  l_ioevent* srev = 0;

  if (rxev->masks == 0) return;
  if (!(srvc = l_master_find_service(rxev->svid))) return;
  if (!(thread = srvc->belong)) return;
  if (!(srvc->mflgs & L_SERVICE_IO_EVENT)) return;

  svmtx = thread->svmtx;

  l_mutex_lock(svmtx);
  srev = srvc->ioev;
  if (!srev || srev->fd != rxev->fd) {
    l_mutex_unlock(svmtx);
    return;
  }

  if (srev->falgs & L_IOEVENT_FLAG_LISTEN) {
    l_mutex_unlock(svmtx);
    l_socket_accept(srev->fd, l_master_accept_connection, srvc);
    return;
  }

  if (srev->flags & L_IOEVENT_FLAG_CONNECT) {
    srev->flags &= (~L_EVENT_FLAG_CONNECT);
    l_mutex_unlock(svmtx);
    l_master_send_connrsp_message(srvc->dstid, srev);
    return;
  }

  if (srev->masks == 0) {
    l_handle fd = event->fd;
    l_umedit masks = event->masks = rxev->masks;
    l_mutex_unlock(svmtx);
    l_send_ioevent_message(master, srvc->svid, L_MESSAGE_IOEVENT, fd, masks);
    return;
  }

  srev->masks |= rxev->masks;
  l_mutex_unlock(svmtx);
}

static void l_master_messages_handle(l_squeue* frmq) {
  l_thread* master = l_master_thread();
  l_message* msg = 0;
  l_squeue msgq;

  l_squeue_init(&msgq);

  l_thread_lock(master);
  l_squeue_push_queue(&msgq, &master->rxmq);
  l_thread_unlock(master);

  while ((msg = (l_message*)l_squeue_pop(msgq))) {
    switch (msg->type) {
    case L_MESSAGE_START_SERVICE: {
        l_service* srvc = (l_service*)((l_message_ptr*)msg)->ptr;
        /* attach thread first */
        if (!srvc->belong) {
          srvc->belong = l_threadpool_acquire();
        }
        /* then add event */
        if (srvc->ioev) {
          l_ioevent event = *srvc->ioev;
          srvc->ioev->masks = 0; /* clear masks, prepare to receive */
          l_ionfmgr_add(l_master_get_ionfmgr(), &event);
        }
        /* add service online to table */
        l_master_add_service(srvc);
      }
      break;
    case L_MESSAGE_CLOSE_EVENTFD: {
        l_service_message* p = (l_service_message*)msg;
        l_handle fd = p->fd;
        l_service* srvc = 0;

        if ((srvc = l_master_find_service(p->svid))) {
          srvc->mflgs &= (~L_SERVICE_IO_EVENT);
        }

        l_ionfmgr_del(l_master_get_ionfmgr(), fd);
        l_socket_close(fd);
      }
      break;
    case L_MESSAGE_CLOSE_SERVICE: {
        l_service_message* p = (l_service_messgae*)msg;
        l_service* srvc = l_master_del_service(p->svid);
        l_handle fd = p->fd;

        if (fd != -1) {
          l_ionfmgr_del(l_master_get_ionfmgr(), fd);
          l_socket_close(fd);
        }

        if (!srvc) break;
        l_threadpool_release(srvc->belong); /* L_MESSAGE_CLOSE_SRVCRSP is the last msg */
        l_send_message_ptr(l_master_thread(), srvc->svid, L_MESSAGE_CLOSE_SRVCRSP, srvc);
      }
      break;
    default:
      l_loge_s("unknow message id");
      break;
    }

    l_squeue_push(&frmq, &msg->node);
  }
}

void l_master_loop() {
  l_squeue rxmq, frmq;
  l_message* msg = 0;
  l_service* srvc = 0;
  l_thread* thread = 0;
  l_thread** ta = &llg_threads;
  l_squeue* mq = 0;
  int n = llg_num_threads;
  int i = 0;

  mq = (l_squeue)l_raw_calloc(sizeof(l_squeue) * n);
  for (; i < n; ++i) {
    l_squeue_init(mq + i);
  }

  l_squeue_init(&rxmq);
  l_squeue_init(&frmq);

  for (; ;) {
    l_ionfmgr_wait(l_master_get_ionfmgr(), l_master_dispatch_event);
    l_master_messages_handle(&frmq); /* handle master's messages */
    l_master_get_messages(&rxmq); /* messages belong to workers */
    l_squeue_push_queue(&rxmq, &master->txmq); /* master to workers' messages */

    while ((msg = (l_message*)l_squeue_pop(&rxmq))) {
      srvc = l_master_find_service(msg->dstid);
      if (!srvc && msg->type != L_MESSAGE_CLOSE_SRVCRSP) {
        l_squeue_push(&frmq, &msg->node);
        continue;
      }

      if (msg->type == L_MESSAGE_CLOSE_SRVCRSP) {
        srvc = (l_service*)((l_message_ptr*)msg)->ptr;
        thread = srvc->belong;
      } else {
        l_mutex* svmtx = 0;
        thread = srvc->belong;
        svmtx = thread->svmtx;
        l_mutex_lock(svmtx);
        if (srvc->stop_rx_msg) {
          l_mutex_unlock(svmtx);
          l_squeue_push(&frmq, &msg->node);
          continue;
        }
        l_mutex_unlock(svmtx);
      }

      msg->srvc = srvc;
      l_squeue_push(mq + thread->index, &msg->node);
    }

    for (i = 0; i < n; ++i) {
      if (l_squeue_is_empty(mq + i)) continue;
      thread = ta[i];

      l_thread_lock(thread);
      l_squeue_push_queue(&thread->rxmq, mq + i);
      if (thread->msgwait) {
        l_thread_unlock(thread);
        continue;
      }
      thread->msgwait = 1;
      l_thread_unlock(thread);

      l_condv_signal(&thread->condv);
    }

    l_free_message_queue(l_master_thread(), &frmq);
  }
}

static void l_worker_flush_messages(l_thread* self) {
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
    l_master_wakeup();
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
1. 添加event时，所在线程分配一个l_ioevent发送给master创建SERVICE，这个l_ioevent会挂在SERVICE上, master添加event后要将thread的masks清零
2. master收到事件通知时，查看service上挂的l_ioevent，如果masks为0则将l_ioevent添加到线程rxio队列，否则直接更新masks即可
3. 线程处理通知，sock关闭时，将服务上挂载的对应l_ioevent（如果没有chained）释放到自己的free buffer pool，并将sock发给master停止监听并让master关闭sock
4. 工作线程中的服务工作完成时，如果没有chained释放到free buffer pool，如果有chained先断开与service的联系，然后在处理该l_ioevent时释放

***需要标记service能不能释放***
*/

static int ll_is_ioevent_message(l_message* msg) {
  l_umedit type = msg->type;
  return type == L_MESSAGE_CONNIND || type == L_MESSAGE_CONNRSP || type == L_MESSAGE_IOEVENT;
}

void l_worker_loop() {
  l_squeue msgq, frmq;
  l_service* srvc = 0;
  l_message* msg = 0;
  l_thread* thread = 0;

  l_squeue_init(&msgq);
  l_squeue_init(&frmq);

  thread = l_thread_self();

  for (; ;) {
    l_thread_lock(thread);
    while (l_squeue_is_empty(&thread->rxmq)) {
      thread->msgwait = 0;
      l_condv_wait(&thread->condv, &thread->mutex);
    }
    l_squeue_push_queue(&msgq, &thread->rxmq);
    l_thread_unlock(thread);

    while ((msg = (l_message*)l_squeue_pop(&msgq))) {
      srvc = msg->srvc;

      switch (msg->type) {
      case L_MESSAGE_CONNIND: case L_MESSAGE_CONNRSP: case L_MESSAGE_IOEVENT: {
          l_mutex* svmtx = thread->svmtx;
          if (!(srvc->wflgs & L_SERVICE_IO_EVENT)) {
            l_squeue_push(&frmq, &msg->node);
            continue;
          }
          l_mutex_lock(svmtx);
          ((l_ioevent_message*)msg)->masks = srvc->ioev->masks;
          srvc->ioev->masks = 0;
          l_mutex_unlock(svmtx);
        }
        break;
      case L_MESSAGE_CLOSE_SRVCRSP:
        l_thread_free_buffer(thread, srvc);
        l_squeue_push(&frmq, &msg->node);
        continue;
      default:
        break;
      }

      if (srvc->wflgs & L_SERVICE_CLOSING) {
        l_squeue_push(&frmq, &msg->node);
        continue;
      }

      srvc->entry(srvc, msg);
      l_squeue_push(&frmq, &msg->node);

      if (!(srvc->flag & L_SERVICE_CLOSING)) {
        continue;
      }

      if (srvc->co) {
        ll_free_coroutine(thread, srvc->co);
        srvc->co = 0;
      }

      l_thread_lock(thread);
      srvc->stop_rx_msg = 1;
      l_thread_unlock(thread);

      l_close_event(srvc);
      l_send_service_message(thread, L_SERVICE_MASTER_ID, L_MESSAGE_CLOSE_SERVICE, srvc->svid, -1);
    }

    l_free_message_queue(thread, &frmq);
    ll_thread_flush_messages(thread);
  }
}

static void l_master_parse_cmdline(int argc, char** argv) {
  (void)argc;
  (void)argv;
}

int startmainthread(int (*start)()) {
  return startmainthread(start, 0, 0);
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
  l_master_parse_cmdline(argc, argv);
  n = start();

  /* clean */
  l_thread_free(&master);
  l_global_free(&G);
  return n;
}

int l_master_start(int (*start)(), int argc, char** argv) {
  return startmainthreadcv(start, argc, argv);
}

