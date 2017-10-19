#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#define L_LIBRARY_IMPL
#define l_buffer_ptr(b) ((L_BUFHEAD*)b->p)
#define l_service_ptr(b) ((l_service*)((b)->p))
#include "core/master.h"
#include "core/state.h"

/**
 * config
 */

typedef struct {
  int workers;
  int service_table_size;
  l_int log_buffer_size;
  l_int thread_max_free_memory;
  l_byte logfile[FILENAME_MAX+1];
  l_byte* prefixend;
  lua_State* L;
} l_config;

static int
l_set_logfile_prefix(void* pconf, l_strt name)
{
  l_config* conf = (l_config*)pconf;
  l_int len = name.end - name.start;
  if (len <= 0 || len > FILENAME_MAX) {
    return false;
  }
  l_copy_n(name.start, len, conf->logfile);
  conf->logfile[len] = 0;
  conf->prefixend = conf->logfile + len;
  return true;
}

static l_config*
l_config_create()
{
  l_config* conf = (l_config*)l_raw_calloc(sizeof(l_config));
  conf->L = l_new_luastate();

  conf->workers = lucy_intconf(conf->L, "workers");
  if (conf->workers < 0) {
    conf->workers = 0;
  }

  conf->log_buffer_size = lucy_intconf(conf->L, "log_buffer_size");
  if (conf->log_buffer_size < BUFSIZ) {
    conf->log_buffer_size = BUFSIZ;
  }
  else if (conf->log_buffer_size + BUFSIZ >= L_MAX_RWSIZE) {
    conf->log_buffer_size = L_MAX_RWSIZE - BUFSIZ - 1;
  }
  conf->log_buffer_size = (((conf->log_buffer_size - 1) / BUFSIZ) + 1) * BUFSIZ;

  conf->service_table_size = lucy_intconf(conf->L, "service_table_size");
  if (conf->service_table_size < 10) {
    conf->service_table_size = 10;
  }

  conf->thread_max_free_memory = lucy_intconf(conf->L, "thread_max_free_memory");
  if (conf->thread_max_free_memory < 1024) {
    conf->thread_max_free_memory = 1024;
  }

  if (!lucy_strconf(conf->L, l_set_logfile_prefix, conf, "logfile_prefix")) {
    /* if get from config file failed, set the default name prefix */
    l_set_logfile_prefix(conf, l_strt_literal("logcat"));
  }

  return conf;
}

static void
l_config_free(l_config* conf)
{
  if (conf->L) {
    l_close_luastate(conf->L);
    conf->L = 0;
  }

  l_raw_mfree(conf);
}

/**
 * thread
 */

typedef struct l_buffer {
  void* p;
} l_buffer;

typedef struct {
  l_squeue queue; /* free buffer q */
  l_int size;  /* size of the queue */
  l_int frmem; /* free memory size */
  l_int limit; /* free memory limit */
} l_freebq;

typedef struct {
  l_mutex mtxa;
  l_mutex mtxb;
  l_condv cnda;
  l_squeue qa;
  l_squeue qb;
  l_squeue qc;
  l_freebq frbq;
} l_thrblock;

typedef struct l_thread {
  l_linknode node;
  l_umedit weight;
  l_ushort index;
  /* shared with master */
  l_mutex* svmtx;
  l_mutex* mutex;
  l_condv* condv;
  l_squeue* rxmq;
  int msgwait;
  /* thread own use */
  lua_State* L;
  l_squeue* txmq;
  l_squeue* txms;
  l_string log;
  l_file logfile;
  l_freebq* freebq;
  l_thrid id;
  int (*start)();
  l_thrblock* block;
} l_thread;

L_GLOBAL l_thrkey l_thrkey_g;
L_THREAD_LOCAL(l_thread* l_self_thread);

L_GLOBAL int l_initialized = false;
L_GLOBAL l_thread l_master_thread;
L_GLOBAL int l_num_workers;
L_GLOBAL l_thread* l_worker_thread;
L_GLOBAL l_priorq l_thread_pool;

static void
l_thread_lock(l_thread* self)
{
  l_mutex_lock(self->mutex);
}

static void
l_thread_unlock(l_thread* self)
{
  l_mutex_unlock(self->mutex);
}

extern l_byte* l_string_print_ulong(l_ulong n, l_byte* p);
L_PRIVAT void l_string_initLog(l_string* log, l_int limit, l_thread* hint);

static void
l_thread_initLog(l_thread* thread, l_config* conf)
{
  l_byte* suffix = 0;

  if (l_strt_equal(l_strt_literal("stdout"), l_strt_c(conf->logfile)) ||
      l_strt_equal(l_strt_literal("stderr"), l_strt_c(conf->logfile))) {
    thread->logfile.stream = stdout;
    thread->log.p = 0;
    return;
  }

  l_string_initLog(&thread->log, conf->log_buffer_size, thread);

  suffix = conf->prefixend;
  *suffix++ = '_';
  suffix = l_string_print_ulong(thread->index, suffix);
  suffix = l_copy_n(".txt", 4, suffix);
  *suffix = 0;
  thread->logfile = l_file_openAppendUnbuffered(conf->logfile);
  l_file_write(&thread->logfile, l_strt_literal("--------" L_NEWLINE));
}

L_EXTERN void
l_thread_flushLog(l_thread* thread)
{
  l_string* log = &thread->log;
  if (!log->p || l_string_isEmpty(log)) {
    return;
  }

  l_file_write(&thread->logfile, l_string_strt(&thread->log));
  l_string_clear(log);
}

static void
l_thread_freeLog(l_thread* thread)
{
  l_thread_flushLog(thread);

  if (thread->log.p) {
    l_string_free(&thread->log, thread);
  }

  if (thread->logfile.stream != stdout && thread->logfile.stream != stderr) {
    l_file_close(&thread->logfile);
  }
}

L_PRIVAT void
l_master_writeLog(l_string* s)
{
  l_thread_flushLog((l_thread*)((l_byte*)s - offsetof(l_thread, log)));
}

L_PRIVAT void
l_master_flushLog(l_thread* hint)
{
  if (hint || (l_initialized && (hint = l_thread_self()))) {
    l_thread_flushLog(hint);
  }
}

L_PRIVAT l_string*
l_master_startLog(const l_byte* tag, l_thread* hint)
{
  l_string* log = 0;

  if (hint || (l_initialized && (hint = l_thread_self()))) {
    log = &hint->log;
    if (!log->p) log = 0;
  }

  l_string_format_s(log, l_strt_c(tag), 0);
  l_string_format_u(log, hint ? hint->index : 0, L_PRECISE(2));
  l_string_format_s(log, l_strt_literal(" "), 0);
  return log;
}

static void
l_thread_init(l_thread* t, l_config* conf)
{
  l_thrblock* b = 0;

  if (t->block)
    return; /* already initialized */

  t->weight = 0;
  t->msgwait = 0;

  t->block = l_raw_malloc(sizeof(l_thrblock));
  b = t->block;

  t->svmtx = &b->mtxa;
  t->mutex = &b->mtxb;
  t->condv = &b->cnda;
  l_mutex_init(t->svmtx);
  l_mutex_init(t->mutex);
  l_condv_init(t->condv);

  t->rxmq = &b->qa;
  t->txmq = &b->qb;
  t->txms = &b->qc;
  l_squeue_init(t->rxmq);
  l_squeue_init(t->txmq);
  l_squeue_init(t->txms);

  t->freebq = &b->frbq;
  l_zero_n(t->freebq, sizeof(l_freebq));
  l_squeue_init(&b->frbq.queue);
  b->frbq.limit = conf->thread_max_free_memory;

  l_thread_initLog(t, conf);
}

static void
l_thread_free(l_thread* t)
{
  l_smplnode* node = 0;
  l_squeue* frbq = 0;
  l_squeue msgq;
  l_squeue_init(&msgq);

  if (!t->block)
    return; /* already freed */

  /* free all messages */

  l_thread_lock(t);
  l_squeue_pushQueue(&msgq, t->rxmq);
  l_thread_unlock(t);

  l_squeue_pushQueue(&msgq, t->txmq);
  l_squeue_pushQueue(&msgq, t->txms);

  while ((node = l_squeue_pop(&msgq))) {
    l_raw_mfree(node);
  }

  /* free all buffers */

  frbq = &t->freebq->queue;
  while ((node = l_squeue_pop(frbq))) {
    l_raw_mfree(node);
  }

  /* free locks */

  l_mutex_free(t->svmtx);
  l_mutex_free(t->mutex);
  l_condv_free(t->condv);

  /* others */

  l_thread_freeLog(t);

  if (t->L) {
    l_close_luastate(t->L);
    t->L = 0;
  }

  l_raw_mfree(t->block);
  t->block = 0;
}

static void*
l_thread_func(void* para)
{
  int n = 0;
  l_thread* self = (l_thread*)para;
#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = self;
#endif
  l_thrkey_setData(&l_thrkey_g, self);
  n = self->start();
#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = 0;
#endif
  l_thrkey_setData(&l_thrkey_g, 0);
  return (void*)(l_int)n;
}

static int
l_thread_start(l_thread* self, int (*start)())
{
  self->start = start;
  return l_raw_thread_create(&self->id, l_thread_func, self);
}

L_EXTERN void
l_thread_exit()
{
  l_raw_thread_exit();
}

L_EXTERN int
l_thread_join(l_thread* self)
{
  return l_raw_thread_join(&self->id);
}

static l_byte*
l_buffer_start(l_buffer* buffer)
{
  return (l_byte*)(l_buffer_ptr(buffer) + 1);
}

static l_int
l_buffer_capacity(l_buffer* buffer)
{
  return l_buffer_ptr(buffer)->bsize - sizeof(L_BUFHEAD);
}

L_PRIVAT int /* if return false, keep buffer unchanged */
l_buffer_ensureCapacity(l_buffer* buffer, l_int capacity)
{
  l_int oldsize = l_buffer_ptr(buffer)->bsize;
  void* newbuffer = 0;

  if (capacity < (int)sizeof(L_BUFHEAD))
    capacity = sizeof(L_BUFHEAD);

  if (!(newbuffer = l_raw_ralloc(buffer->p, oldsize, capacity))) {
    return false;
  }

  buffer->p = newbuffer;
  l_buffer_ptr(buffer)->bsize = capacity;
  return true;
}

L_PRIVAT int /* returned buffer is initialized to 0 */
l_buffer_init(l_buffer* buffer, l_int size, l_thread* hint)
{
  if (hint) { /* hint can only be current thread */

    l_freebq* q = hint->freebq;

    if ((buffer->p = l_squeue_pop(&q->queue))) { /* current thread has free buffer */

      if (!l_buffer_ensureCapacity(buffer, size)) { /* return unchanged buffer back if failed */
        l_squeue_push(&q->queue, &(l_buffer_ptr(buffer)->node));
        return false;
      }

      q->size -= 1;
      q->frmem -= l_buffer_ptr(buffer)->bsize;

      return true;
    }

    /* else go down to alloc raw memory */
  }

  if (size < (int)sizeof(L_BUFHEAD))
    size = sizeof(L_BUFHEAD);

  if ((buffer->p = l_raw_calloc(size))) {
    l_buffer_ptr(buffer)->bsize = size;
    return true;
  }

  return false;
}

L_PRIVAT void
l_buffer_free(l_buffer* buffer, l_thread* hint)
{
  l_freebq* q = 0;

  if (!buffer->p)
    return; /* already freed */

  if (!hint) {
    l_raw_mfree(buffer->p);
    buffer->p = 0;
    return;
  }

  q = hint->freebq; /* hint can only be current thread */

  l_squeue_push(&q->queue, &(l_buffer_ptr(buffer)->node));
  q->size += 1;
  q->frmem += l_buffer_ptr(buffer)->bsize;

  while (q->frmem > q->limit) {
    if (!(buffer->p = l_squeue_pop(&q->queue))) {
      break;
    }
    q->size -= 1;
    q->frmem -= l_buffer_ptr(buffer)->bsize;
    l_raw_mfree(buffer->p);
  }

  buffer->p = 0;
}

static l_thread*
l_thread_acquire()
{
  l_thread* thread = 0;

  thread = (l_thread*)l_priorq_pop(&l_thread_pool);
  if (!thread) {
    return 0;
  }

  thread->weight += 1;
  l_priorq_push(&l_thread_pool, &thread->node);
  return thread;
}

static void
l_thread_acquireSpecific(l_thread* thread)
{
  if (thread == l_thread_master()) {
    return;
  }

  thread->weight += 1;
  l_priorq_remove(&l_thread_pool, &thread->node);
  l_priorq_push(&l_thread_pool, &thread->node);
}

static void
l_thread_release(l_thread* thread)
{
  if (thread == l_thread_master()) {
    return;
  }

  thread->weight -= 1;
  l_priorq_remove(&l_thread_pool, &thread->node);
  l_priorq_push(&l_thread_pool, &thread->node);
}

/**
 * message
 */

#define l_message_ptr(b) ((l_message*)(b)->p)
#define L_MSGID_SRVC_START_REQ  0x01
#define L_MSGID_SRVC_START_RSP  0x02
#define L_MSGID_SRVC_CLOSE_REQ  0x03
#define L_MSGID_SRVC_CLOSE_RSP  0x04
#define L_MSGID_SRVC_CLOSE_SOCK 0x05
#define L_MSGID_START_BOOTSTRAP 0x06
#define L_MSGID_MASTER_EXIT_REQ 0x07
#define L_MSGID_WORKER_EXIT_REQ 0x08
#define L_MSGID_WORKER_EXIT_RSP 0x09
#define L_MSGID_SOCK_EVENT_IND  0x0a
#define L_MSGID_SOCK_CONN_RSP   0x0b
#define L_MSGID_SOCK_CONN_IND   0x0c
#define L_MSGID_SOCK_CONNV6_IND 0x0d
#define L_MESSAGE_START_ID      0xffff+1

#define L_SERVICE_MASTER    0x00
#define L_SERVICE_WORKER    0x01
#define L_SERVICE_BOOTSTRAP 0x02
#define L_SERVICE_START_ID  0xffff+1

L_GLOBAL l_squeue l_msg_rxq;
L_GLOBAL l_mutex l_msg_mtx;
L_GLOBAL l_eventmgr l_eventmgr_g;

static void
l_master_wakeup()
{
  /* wakeup master to handle the message */
  l_eventmgr_wakeup(&l_eventmgr_g);
}

L_EXTERN l_message*
l_message_create(l_int size, l_thread* hint)
{
  l_buffer buffer;

  if (size < (l_int)sizeof(l_message)) {
    l_loge_1("size %d", ld(size));
    return 0;
  }

  if (!l_buffer_init(&buffer, size, hint)) {
    return 0;
  }

  return l_message_ptr(&buffer);
}

L_EXTERN void
l_message_free(l_message* msg, l_thread* hint)
{
  l_buffer buffer = {&msg->HEAD};
  l_buffer_free(&buffer, hint);
}

L_EXTERN void
l_message_freeQueue(l_squeue* mq, l_thread* hint)
{
  l_message* msg = 0;
  while ((msg = (l_message*)l_squeue_pop(mq))) {
    l_message_free(msg, hint);
  }
}

static void /* send message to dest service from current thread */
l_message_send_msg_impl(l_thread* from, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64, l_message* msg)
{
  msg->dest = destid;
  msg->msgid = msgid;
  msg->data = u32;
  msg->extra = u64;

  if (from->index != 0 && (destid >> 32) == from->index) {
    l_thread_lock(from);
    l_squeue_push(from->rxmq, &msg->HEAD.node);
    l_thread_unlock(from);
    return;
  }

  if ((destid & 0xffffffff) == 0) { /* master svid is 0 */
    l_squeue_push(from->txms, &msg->HEAD.node);
  } else {
    l_squeue_push(from->txmq, &msg->HEAD.node);
  }
}

static void /* worker flush messages to global in worker thread */
l_worker_flushMessages(l_thread* self)
{
  l_mutex* mtx = &l_msg_mtx;
  l_squeue* txmq = self->txmq;
  l_squeue* txms = self->txms;
  int sent = false;

  if (!l_squeue_isEmpty(txms)) {
    l_thread* master = l_thread_master();
    l_thread_lock(master);
    l_squeue_pushQueue(master->rxmq, txms);
    l_thread_unlock(master);
    sent = true;
  }

  if (!l_squeue_isEmpty(txmq)) {
    l_mutex_lock(mtx);
    l_squeue_pushQueue(&l_msg_rxq, txmq);
    l_mutex_unlock(mtx);
    sent = true;
  }

  if (sent) {
    l_master_wakeup();
  }
}

static void /* master get messages from global in master thread */
l_master_getMessages(l_thread* master, l_squeue* outq)
{
  l_mutex* mtx = &l_msg_mtx;
  l_mutex_lock(mtx);
  l_squeue_pushQueue(outq, &l_msg_rxq); /* messages from other threads */
  l_mutex_unlock(mtx);
  l_squeue_pushQueue(outq, master->txmq); /* messages master send to workers */
}

L_EXTERN void
l_message_send(l_thread* from, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64, l_message* msg)
{
  l_message_send_msg_impl(from, destid, msgid + L_MESSAGE_START_ID, u32, u64, msg);
}

static void
l_message_send_to_master(l_thread* from, l_umedit msgid, l_umedit u32, l_ulong u64, l_message* msg)
{
  l_message_send_msg_impl(from, L_SERVICE_MASTER, msgid, u32, u64, msg);
}

static void
l_message_send_impl(l_thread* thread, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64)
{
  l_message* msg = l_message_create(sizeof(l_message), thread);
  l_message_send_msg_impl(thread, destid, msgid, u32, u64, msg);
}

L_EXTERN void
l_message_sendData(l_thread* thread, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64)
{
  l_message_send_impl(thread, destid, msgid + L_MESSAGE_START_ID, u32, u64);
}

static void
l_message_send_data_to_master(l_thread* thread, l_umedit msgid, l_umedit u32, l_ulong u64)
{
  l_message_send_impl(thread, L_SERVICE_MASTER, msgid, u32, u64);
}

static void
l_message_startService(l_thread* thread, l_service* srvc)
{
  l_message_send_data_to_master(thread, L_MSGID_SRVC_START_REQ, 0, l_msg_castp(srvc));
}

static void
l_message_closeService(l_thread* thread, l_umedit svid, l_filedesc sock)
{
  l_message_send_data_to_master(thread, L_MSGID_SRVC_CLOSE_REQ, svid, l_msg_castfd(sock));
}

static void
l_message_closeServiceSocket(l_thread* thread, l_umedit svid, l_filedesc sock)
{
  l_message_send_data_to_master(thread, L_MSGID_SRVC_CLOSE_SOCK, svid, l_msg_castfd(sock));
}

static void
l_message_masterExit(l_thread* thread)
{
  l_message_send_data_to_master(thread, L_MSGID_MASTER_EXIT_REQ, 0, 0);
}

static void
l_message_startBootstrap(l_thread* thread, int (*bootstrap)())
{
  l_message_send_impl(thread, L_SERVICE_BOOTSTRAP, L_MSGID_START_BOOTSTRAP, 0, l_msg_castfunc(bootstrap));
}

/**
 * service
 *
 * service is single linked in the service hash table after it created.
 * service is freed as a free buffer after it is completed.
 */

#define L_SERVICE_STARTED   0x01
#define L_SERVICE_CLOSING   0x02
#define L_SERVICE_STOPRX    0x04
#define L_SERVICE_SOCKET    0x08

typedef struct l_service {
  L_BUFHEAD HEAD;
  l_ioevent* ioev; /* guard by svmtx */
  l_ioevent event; /* guard by svmtx */
  l_uint flags; /* shared flags stop_rx_msg service is closing, guard by svmtx */
  /* thread own use */
  l_umedit flagw; /* only accessed by a worker */
  l_ulong svid; /* only set once when init, so can freely access it */
  l_thread* thread; /* only set once when init, so can freely access it */
  int (*entry)(l_service*, l_message*); /* service entry function */
  void (*destroy)(l_service*); /* destroy function for child structure's fields if necessary */
  /* coroutine */
  lua_State* co;
  int (*func)(l_service*);
  int (*kfunc)(l_service*);
  int coref;
} l_service;

typedef struct {
  l_smplnode node;
} l_srvcslot;

typedef struct {
  l_umedit nslot;
  l_umedit nelem;
  l_srvcslot* slot;
} l_srvctable;

L_GLOBAL l_mutex l_srvc_mtx;
L_GLOBAL l_umedit l_svid_seed; /* shared by all threads */
L_GLOBAL l_srvctable l_srvc_table; /* only accessed by master */

L_EXTERN l_ulong
l_service_id(l_service* srvc)
{
  return srvc->svid;
}

static l_umedit
l_service_id_for_lookup(l_service* srvc)
{
  return (srvc->svid & 0xffff);
}

static int
l_srvctable_init(l_srvctable* self, l_byte sizebits)
{
  self->nelem = 0;

  if (sizebits > 30) {
    self->nslot = 0;
    self->slot = 0;
    l_loge_1("slots 2^%d", ld(sizebits));
    return false;
  }

  self->nslot = (1 << sizebits);
  self->slot = (l_srvcslot*)l_raw_calloc(sizeof(l_srvcslot)*self->nslot);
  return true;
}

static l_smplnode*
llheadnode(l_srvctable* self, l_umedit svid)
{
  return &(self->slot[svid & (self->nslot - 1)].node);
}

static void
l_srvctable_add(l_srvctable* self, l_smplnode* elem)
{
  l_smplnode* slot = 0;
  if (elem == 0) return;
  slot = llheadnode(self, l_service_id_for_lookup((l_service*)elem));
  elem->next = slot->next;
  slot->next = elem;
  self->nelem += 1;
  if (self->nelem > self->nslot) {
    l_logw_2("nelem %d nslot %d", ld(self->nelem), ld(self->nslot));
  }
}

static l_service*
l_srvctable_find(l_srvctable* self, l_umedit svid)
{
  l_smplnode* slot = 0;
  l_smplnode* srvc = 0;
  slot = llheadnode(self, svid);
  srvc = slot->next;
  while (srvc) {
    if (l_service_id_for_lookup((l_service*)srvc) == svid) return (l_service*)srvc;
    srvc = srvc->next;
  }
  return 0;
}

static l_service*
l_srvctable_del(l_srvctable* self, l_umedit svid)
{
  l_smplnode* elem = 0;
  l_smplnode* node = 0;
  elem = llheadnode(self, svid);
  while (elem->next) {
    node = elem->next;
    if (l_service_id_for_lookup((l_service*)node) == svid) {
      elem->next = node->next;
      self->nelem -= 1;
      return (l_service*)node;
    }
    elem = node;
  }
  return 0;
}

static void
l_srvctable_foreach(l_srvctable* self, void (*cb)(l_service*))
{
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

static void
l_srvctable_clear(l_srvctable* self, l_allocfunc func)
{
  l_srvcslot* slot = self->slot;
  l_srvcslot* end = slot + self->nslot;
  l_smplnode* head = 0;
  l_smplnode* first = 0;
  if (self->slot == 0) return;
  for (; slot < end; ++slot) {
    head = &slot->node;
    while (head->next) {
      first = head->next;
      head->next = first->next;
      if (func) l_mfree(func, first);
    }
  }
}

static void
l_srvctable_free(l_srvctable* self, l_allocfunc func)
{
  if (!self->slot) return;
  l_srvctable_clear(self, func);
  l_raw_mfree(self->slot);
  self->slot = 0;
  self->nelem = 0;
}

static void
l_master_addService(l_service* srvc)
{
  l_srvctable_add(&l_srvc_table, &srvc->HEAD.node);
}

static l_service*
l_master_findService(l_umedit svid)
{
  return l_srvctable_find(&l_srvc_table, svid);
}

static l_service*
l_master_delService(l_umedit svid)
{
  return l_srvctable_del(&l_srvc_table, svid);
}

static l_umedit
l_master_newSvid()
{
  l_mutex* mtx = &l_srvc_mtx;
  l_umedit svid = 0;

  l_mutex_lock(mtx);
  if (++l_svid_seed < L_SERVICE_START_ID) {
    l_svid_seed = L_SERVICE_START_ID;
  }
  svid = l_svid_seed;
  l_mutex_unlock(mtx);

  return svid;
}

L_EXTERN l_service*
l_service_create(l_int size, int (*entry)(l_service*, l_message*), void (*destroy)(l_service*)) {
  l_buffer buffer;
  l_thread* thread = 0;

  if (size < (l_int)sizeof(l_service)) {
    l_loge_1("size %d", ld(size));
    return 0;
  }

  thread = l_thread_self();
  if (!l_buffer_init(&buffer, size, thread)) {
    return 0;
  }

  l_service_ptr(&buffer)->svid = l_master_newSvid();
  l_service_ptr(&buffer)->thread = thread;
  l_service_ptr(&buffer)->entry = entry;
  l_service_ptr(&buffer)->destroy = destroy;
  return l_service_ptr(&buffer);
}

L_EXTERN int
l_service_free(l_service* srvc)
{
  l_buffer buffer;

  if (srvc->flags | L_SERVICE_STARTED) {
    l_loge_s("already started");
    return false;
  }

  buffer.p = &srvc->HEAD;
  l_buffer_free(&buffer, l_thread_self());
  return true;
}

L_EXTERN void
l_service_start(l_service* srvc)
{
  l_thread* self = srvc->thread;
  srvc->thread = 0;
  srvc->flagw |= L_SERVICE_STARTED;
  l_message_startService(self, srvc);
}

L_EXTERN void
l_service_startEx(l_service* srvc, l_thread* thread)
{
  l_thread* self = srvc->thread;
  if (thread) srvc->thread = thread; /* else keep srvc->thread as current thread */
  srvc->flagw |= L_SERVICE_STARTED;
  l_message_startService(self, srvc);
}

static l_service*
l_service_setSocket(l_service* srvc, l_filedesc sock, l_ushort masks, l_ushort flags)
{
  srvc->ioev = &srvc->event;
  srvc->flagw |= L_SERVICE_SOCKET;
  srvc->flags |= L_SERVICE_SOCKET;

  srvc->ioev->fd = sock;
  srvc->ioev->udata = l_service_id_for_lookup(srvc);
  srvc->ioev->masks = masks;
  srvc->ioev->flags = flags;
  return srvc;
}

L_EXTERN l_service*
l_service_setListenSocket(l_service* srvc, l_filedesc sock)
{
  return l_service_setSocket(srvc, sock, L_SOCKET_READ, L_SOCKET_FLAG_LISTEN);
}

L_EXTERN l_service*
l_service_setInitiateSocket(l_service* srvc, l_filedesc sock)
{
  return l_service_setSocket(srvc, sock, L_SOCKET_RDWR, L_SOCKET_FLAG_CONNECT);
}

L_EXTERN l_service*
l_service_setReceiveSocket(l_service* srvc, l_filedesc sock)
{
  return l_service_setSocket(srvc, sock, L_SOCKET_RDWR, 0);
}

void l_service_close(l_service* srvc) {
  srvc->flagw |= L_SERVICE_CLOSING;
}

void l_service_closeSocket(l_service* srvc) {
  l_thread* thread = srvc->thread;
  l_filedesc sock = l_filedesc_empty();

  if (srvc->flagw & L_SERVICE_SOCKET) {
    l_mutex* svmtx = thread->svmtx;
    l_ioevent* ioev = 0;

    l_mutex_lock(svmtx);
    ioev = srvc->ioev;
    /* store fd to send to mater to close */
    sock = ioev->fd;
    /* reset fd as closed */
    ioev->fd = l_filedesc_empty();
    /* if thread still has io message in the pending queue,
    when to handle it, this message will be ignored due to
    no event attached to service from now */
    srvc->ioev = 0;
    l_mutex_unlock(svmtx);

    srvc->flagw &= (~L_SERVICE_SOCKET);
  }

  if (l_filedesc_isEmpty(&sock)) {
    return;
  }

  l_message_closeServiceSocket(thread, srvc->svid, sock);
}

/**
 * task dispatch
 */

static int
l_thread_less(void* lhs, void* rhs)
{
  return ((l_thread*)lhs)->weight < ((l_thread*)rhs)->weight;
}

static void
l_master_init()
{
  int i = 0;
  l_strt prefix;
  l_config* conf = 0;
  l_thread* master = &l_master_thread;
  l_thread* thread = 0;

  if (l_initialized) return;

  l_thrkey_init(&l_thrkey_g);

  /* config */

  conf = l_config_create();
  prefix = l_strt_from(conf->logfile, conf->prefixend);
  l_logm_5("workers %d log_buffer_size %d service_table_size 2^%d thread_max_free_memory %d logfile_prefix %strt",
      ld(conf->workers), ld(conf->log_buffer_size), ld(conf->service_table_size), ld(conf->thread_max_free_memory), lstrt(&prefix));

  /* master thread */

#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = 0;
#endif
  l_thrkey_setData(&l_thrkey_g, 0);

  l_thread_init(master, conf);
  master->id = l_raw_thread_self(); /* master thread created by os */
  master->L = conf->L;
  conf->L = 0;

#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = master;
#endif
  l_thrkey_setData(&l_thrkey_g, master);

  /* worker thread pool */

  l_priorq_init(&l_thread_pool, l_thread_less);

  if (conf->workers > 0) {
    l_num_workers = conf->workers;
    l_worker_thread = (l_thread*)l_raw_calloc(sizeof(l_thread) * conf->workers);

    for (i = 0; i < conf->workers; ++i) {
      thread = l_worker_thread + i;
      thread->index = i + 1; /* worker index should not 0 */
      l_thread_init(thread, conf);
      thread->L = l_new_luastate();
      l_priorq_push(&l_thread_pool, &thread->node);
    }

  } else {
    l_num_workers = 0;
    l_worker_thread = 0;
  }

  /* socket */

  l_socket_init();
  l_eventmgr_init(&l_eventmgr_g);

  /* message */

  l_squeue_init(&l_msg_rxq);
  l_mutex_init(&l_msg_mtx);

  /* service */

  l_mutex_init(&l_srvc_mtx);
  l_svid_seed = L_SERVICE_START_ID;
  l_srvctable_init(&l_srvc_table, conf->service_table_size);

  /* others */

  l_config_free(conf);

  l_initialized = true;
}

static void
l_master_clean()
{
  l_smplnode* node = 0;
  l_thread* thread = 0;
  l_thread* master = l_thread_master();

  if (!l_initialized) return;

  /* socket */

  l_eventmgr_free(&l_eventmgr_g);

  /* clean messages */

  l_mutex_lock(&l_msg_mtx);
  while ((node = l_squeue_pop(&l_msg_rxq))) {
    l_raw_mfree(node);
  }
  l_mutex_unlock(&l_msg_mtx);
  l_mutex_free(&l_msg_mtx);

  /* clean services */

  l_srvctable_free(&l_srvc_table, l_raw_alloc_func);
  l_mutex_free(&l_srvc_mtx);

  /* clean threads */

  l_thread_free(master);

  while ((thread = (l_thread*)l_priorq_pop(&l_thread_pool))) {
    l_thread_free(thread);
  }

  if (l_worker_thread) {
    l_raw_mfree(l_worker_thread);
    l_worker_thread = 0;
  }

  l_num_workers = 0;

  /* others */

  l_thrkey_free(&l_thrkey_g);

#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = 0;
#endif

  l_initialized = false;
}

L_EXTERN l_thread*
l_thread_master()
{
  return &l_master_thread;
}

static void
l_master_acceptConnection(void* ud, l_sockconn* conn)
{
  l_service* sv = (l_service*)ud;
  l_thread* master = l_thread_master();
  int family = l_sockaddr_family(&conn->remote);
  if (family == L_SOCKADDR_IPV4) {

  } else if (family == L_SOCKADDR_IPV6) {
    l_message* msg = l_message_create(sizeof(l_ipv6connind_message), master);
    /* TODO: copy address data */
    l_message_send_msg_impl(master, l_service_id(sv), L_MSGID_SOCK_CONNV6_IND, 0, 0, msg);
  } else {
    l_loge_s("invalid address family");
  }
}

static void
l_master_dispatchEvent(l_ioevent* rxev)
{
  l_thread* master = l_thread_master();
  l_service* srvc = 0;
  l_thread* thread = 0;
  l_mutex* svmtx = 0;
  l_ioevent* srev = 0;

  if (rxev->masks == 0) return;
  if (!(srvc = l_master_findService(rxev->udata))) return;
  if (!(thread = srvc->thread)) return;
  if (!(srvc->flags & L_SERVICE_SOCKET)) return;

  svmtx = thread->svmtx;

  l_mutex_lock(svmtx);
  srev = srvc->ioev;
  if (!srev || !l_filedesc_equal(&srev->fd, &rxev->fd)) {
    l_mutex_unlock(svmtx);
    return;
  }

  if (srev->flags & L_SOCKET_FLAG_LISTEN) {
    l_mutex_unlock(svmtx);
    l_socket_accept(srev->fd, l_master_acceptConnection, srvc);
    return;
  }

  if (srev->flags & L_SOCKET_FLAG_CONNECT) {
    srev->flags &= (~L_SOCKET_FLAG_CONNECT);
    l_mutex_unlock(svmtx);
    l_message_send_impl(master, l_service_id(srvc), L_MSGID_SOCK_CONN_RSP, srev->masks, l_msg_castfd(srev->fd));
    return;
  }

  if (srev->masks == 0) {
    l_filedesc fd = srev->fd;
    l_umedit masks = srev->masks = rxev->masks;
    l_mutex_unlock(svmtx);
    l_message_send_impl(master, l_service_id(srvc), L_MSGID_SOCK_EVENT_IND, masks, l_msg_castfd(fd));
    return;
  }

  srev->masks |= rxev->masks;
  l_mutex_unlock(svmtx);
}

static int
l_worker_handleMessage(l_thread* thread, l_message* msg)
{
  l_service* srvc = 0;
  l_mutex* mtx = 0;
  int threadExit = false;

  srvc = (l_service*)msg->dest;

  switch (msg->msgid) {
  case L_MSGID_SOCK_CONN_IND: case L_MSGID_SOCK_CONN_RSP: case L_MSGID_SOCK_EVENT_IND:
    mtx = thread->svmtx;
    l_mutex_lock(mtx);
    msg->data = srvc->ioev->masks;
    srvc->ioev->masks = 0;
    l_mutex_unlock(mtx);
    break;
  case L_MSGID_SRVC_START_RSP:
    l_logm_1("service %d started", ld(srvc->svid));
    break;
  case L_MSGID_SRVC_CLOSE_RSP: {
      l_buffer buffer = {srvc};
      l_logm_1("service %d closed", ld(srvc->svid));
      l_buffer_free(&buffer, thread);
      return true;
    }
  case L_MSGID_WORKER_EXIT_REQ:
    l_logm_1("worker %d prepare exit", ld(thread->index));
    threadExit = true;
    l_message_send_impl(thread, L_SERVICE_MASTER, L_MSGID_WORKER_EXIT_RSP, thread->index, 0);
    break;
  default:
    break;
  }

  if (threadExit) {
    return false;
  }

  if (!(srvc->flagw & L_SERVICE_CLOSING)) {
    srvc->entry(srvc, msg);
  }

  if (srvc->flagw & L_SERVICE_CLOSING) {
    l_service_free_state(srvc);

    l_thread_lock(thread);
    srvc->flags |= L_SERVICE_STOPRX;
    l_thread_unlock(thread);

    l_service_closeSocket(srvc);
    l_message_closeService(thread, l_service_id_for_lookup(srvc), l_filedesc_empty());
  }

  return true;
}

static int
l_master_handleMessage(l_squeue* frmq)
{
  l_thread* master = l_thread_master();
  l_message* msg = 0;
  l_squeue msgq;
  int exited_workers = 0;
  int masterExit = false;

  l_squeue_init(&msgq);

  l_thread_lock(master);
  l_squeue_pushQueue(&msgq, master->rxmq);
  l_thread_unlock(master);

  l_squeue_pushQueue(&msgq, master->txms);

  while ((msg = (l_message*)l_squeue_pop(&msgq))) {
    switch (msg->msgid) {
    case L_MSGID_SRVC_START_REQ: {
        l_service* srvc = (l_service*)msg->extra;
        /* attach thread first */
        if (srvc->thread) {
          l_thread_acquireSpecific(srvc->thread);
        } else {
          srvc->thread = l_thread_acquire();
        }

        if (!srvc->thread) { /* if no worker threads, just use master thread */
          srvc->thread = l_thread_master();
        }

        srvc->svid = (((l_ulong)srvc->thread->index) << 32) | l_service_id_for_lookup(srvc);

        l_logm_1("start service %d", ld(srvc->svid));

        /* then add event */
        if (srvc->ioev) {
          l_ioevent event = *srvc->ioev;
          srvc->ioev->masks = 0; /* clear masks, prepare to receive */
          l_eventmgr_add(&l_eventmgr_g, &event);
        }
        /* add service online to table */
        l_master_addService(srvc);
        /* send confirm back */
        l_message_send_impl(master, l_service_id(srvc), L_MSGID_SRVC_START_RSP, 0, 0);
      }
      break;
    case L_MSGID_SRVC_CLOSE_SOCK: {
        l_filedesc fd = l_msg_getfd(msg);
        l_service* srvc = 0;
        if ((srvc = l_master_findService(msg->data))) {
          srvc->flags &= (~L_SERVICE_SOCKET);
        }

        l_eventmgr_del(&l_eventmgr_g, fd);
        l_socket_close(fd);
      }
      break;
    case L_MSGID_SRVC_CLOSE_REQ: {
        l_service* srvc = l_master_delService(msg->data);
        l_filedesc fd = l_msg_getfd(msg);

        l_logm_1("close service %d", ld(msg->data));

        if (!l_filedesc_isEmpty(&fd)) {
          l_eventmgr_del(&l_eventmgr_g, fd);
          l_socket_close(fd);
        }

        if (!srvc) break;
        l_thread_release(srvc->thread); /* L_MSGID_SRVC_CLOSE_RSP is the last msg */
        l_message_send_impl(master, l_service_id(srvc), L_MSGID_SRVC_CLOSE_RSP, 0, l_msg_castp(srvc));
      }
      break;
    case L_MSGID_MASTER_EXIT_REQ: {
        int i = 0;
        l_thread* thread = l_worker_thread;
        l_logm_s("prepare exit");

        if (!thread || l_num_workers <= 0) { /* no workers, exit directly */
          masterExit = true;
          break;
        }

        for (; i < l_num_workers; ++i) {
          if (thread[i].index == 0) continue; /* already exit */
          /* TODO: how to send to worker without specified service id */
          l_message_send_impl(master, (((l_ulong)thread[i].index) << 32) | L_SERVICE_WORKER, L_MSGID_WORKER_EXIT_REQ, 0, 0);
        }
      }
      break;
    case L_MSGID_WORKER_EXIT_RSP: {
        int i = msg->data;
        l_worker_thread[i].index = 0;
        ++exited_workers;
        l_logm_3("worker %d exited %d/%d", ld(i), ld(exited_workers), ld(l_num_workers));
        if (exited_workers == l_num_workers) {
          masterExit = true;
        }
      }
      break;
    default:
      if (l_worker_thread) {
        l_loge_1("unknow message %d", ld(msg->msgid));
      } else {
        l_worker_handleMessage(master, msg);
      }
      break;
    }

    l_squeue_push(frmq, &msg->HEAD.node);
    if (masterExit) return false;
  }

  return true;
}

static int
l_bootstrap_service_proc(l_service* self, l_message* msg)
{
  (void)self;
  switch (msg->msgid) {
  case L_MSGID_SRVC_START_RSP:
    l_logm_s("bootstrap startup");
    break;
  case L_MSGID_START_BOOTSTRAP:
    l_logm_1("bootstrap service %d", ld(self->svid));
    if (msg->extra) {
      int (*func)() = (int (*)())(void*)(l_uint)msg->extra;
      return func();
    }
    break;
  default:
    break;
  }
  return 0;
}

void l_master_exit()
{
  l_message_masterExit(l_thread_self());
}

static int
l_master_loop(int (*start)())
{
  l_squeue rxmq, frmq;
  l_message* msg = 0;
  l_service* srvc = 0;
  l_thread* master = l_thread_master();
  l_thread* worker = l_worker_thread;
  l_thread* thread = 0;
  l_uint waitCount = 0;
  l_squeue* mq = 0;
  int i = 0, n = 0;
  int exitCode = 0;

  l_logm_s("master run");

  if (l_num_workers > 0) {
    mq = (l_squeue*)l_raw_calloc(sizeof(l_squeue) * l_num_workers);
    for (n = 0; n < l_num_workers; ++n) { 
      l_squeue_init(mq + n);
    }
    n = l_num_workers;
  }

  l_squeue_init(&rxmq);
  l_squeue_init(&frmq);

  srvc = l_service_create(sizeof(l_service), l_bootstrap_service_proc, 0);
  srvc->svid = L_SERVICE_BOOTSTRAP;
  l_service_start(srvc); /* start bootstrap service */
  l_master_handleMessage(&frmq); /* handle bootstrap start message */
  l_message_startBootstrap(master, start); /* send BOOTSTRAP message */

  for (; ;) {
    if (!l_squeue_isEmpty(master->txms) || !l_squeue_isEmpty(master->txmq)) {
      l_master_wakeup();
    }

    l_logm_1("master T%d wait", ld(++waitCount));
    l_eventmgr_wait(&l_eventmgr_g, l_master_dispatchEvent);
    l_logm_1("master T%d wakeup", ld(waitCount));

    if (!l_master_handleMessage(&frmq)) {
      l_message_freeQueue(&frmq, master);
      exitCode = 0; /* normal exit */
      break;
    }

    l_master_getMessages(master, &rxmq); /* messages need send to workers */

    while ((msg = (l_message*)l_squeue_pop(&rxmq))) {
      if (msg->msgid == L_MSGID_WORKER_EXIT_REQ) {
        l_squeue_push(mq + msg->dest - 1, &msg->HEAD.node); /* msg->dest contains thread index */
        continue;
      }

      srvc = l_master_findService((l_umedit)(msg->dest&0xffff));
      if (!srvc && msg->msgid != L_MSGID_SRVC_CLOSE_RSP) {
        l_squeue_push(&frmq, &msg->HEAD.node);
        continue;
      }

      if (msg->msgid == L_MSGID_SRVC_CLOSE_RSP) {
        srvc = (l_service*)msg->extra;
        thread = srvc->thread;
      } else {
        l_mutex* svmtx = 0;
        thread = srvc->thread;
        svmtx = thread->svmtx;
        l_mutex_lock(svmtx);
        if (srvc->flags & L_SERVICE_STOPRX) {
          l_mutex_unlock(svmtx);
          l_squeue_push(&frmq, &msg->HEAD.node);
          continue;
        }
        l_mutex_unlock(svmtx);
      }

      msg->dest = (l_ulong)(l_uint)srvc;

      if (thread == l_thread_master()) {
        l_squeue_push(master->txms, &msg->HEAD.node);
        continue;
      }

      if (thread->index == 0) { /* dest thread already exit */
        l_squeue_push(&frmq, &msg->HEAD.node);
        continue;
      }

      l_squeue_push(mq + thread->index - 1, &msg->HEAD.node);
    }

    for (i = 0; i < n; ++i) {
      if (l_squeue_isEmpty(mq + i)) continue;
      thread = worker + i;

      if (thread->index == 0) { /* already exit */
        l_squeue_pushQueue(&frmq, mq + i);
        continue;
      }

      l_thread_lock(thread);
      l_squeue_pushQueue(thread->rxmq, mq + i);
      if (thread->msgwait) {
        l_thread_unlock(thread);
        continue;
      }
      thread->msgwait = 1;
      l_thread_unlock(thread);

      l_condv_signal(thread->condv);
    }

    l_message_freeQueue(&frmq, master);
  }

  /* master loop exited */

  if (mq) {
    l_raw_mfree(mq);
  }

  return exitCode;
}

static int
l_worker_start()
{
  l_squeue msgq, frmq;
  l_message* msg = 0;
  l_thread* thread = 0;
  int threadExit = false;

  l_squeue_init(&msgq);
  l_squeue_init(&frmq);
  thread = l_thread_self();

  l_logm_1("worker %d run", ld(thread->index));

  for (; ;) {
    l_thread_lock(thread);
    while (l_squeue_isEmpty(thread->rxmq)) {
      thread->msgwait = 0;
      l_condv_wait(thread->condv, thread->mutex);
    }
    l_squeue_pushQueue(&msgq, thread->rxmq);
    l_thread_unlock(thread);

    while ((msg = (l_message*)l_squeue_pop(&msgq))) {
      if (!l_worker_handleMessage(thread, msg)) {
        threadExit = true;
      }
      l_squeue_push(&frmq, &msg->HEAD.node);
    }

    l_message_freeQueue(&frmq, thread);

    if (threadExit) {
      break;
    }

    l_worker_flushMessages(thread);
  }

  return 0;
}

static void
l_master_parse_command_line(int argc, char** argv)
{
  (void)argc;
  (void)argv;
}

L_EXTERN int
startmainthread(int (*start)())
{
  return startmainthreadcv(start, 0, 0);
}

L_EXTERN int
startmainthreadcv(int (*start)(), int argc, char** argv)
{
  int i = 0;
  l_master_init();
  l_master_parse_command_line(argc, argv);

  l_logm_s("master initialized");

  for (i = 0; i < l_num_workers; ++i) {
    l_thread_start(l_worker_thread + i, l_worker_start);
  }
  l_logm_1("%d-worker started", ld(l_num_workers));

  i = l_master_loop(start);
  l_logm_1("master exited %d", ld(i));

  for (i = 0; i < l_num_workers; ++i) {
    l_thread_join(l_worker_thread + i);
  }
  l_logm_s("workers exited");

  l_master_clean();
  l_logm_s("cleaned up");

  return 0;
}

L_EXTERN void
l_master_test()
{
  l_long data = -100;
  l_ulong udata = data;
  l_assert(sizeof(l_mutex) >= L_MUTEX_SIZE);
  l_assert(sizeof(l_rwlock) >= L_RWLOCK_SIZE);
  l_assert(sizeof(l_condv) >= L_CONDV_SIZE);
  l_assert(sizeof(l_thrkey) >= L_THKEY_SIZE);
  l_assert(sizeof(l_thrid) >= L_THRID_SIZE);
  l_assert(sizeof(void*) <= 8);
  l_assert(udata == ((l_ulong)-100));
  l_assert(((l_long)udata) == -100);
}

