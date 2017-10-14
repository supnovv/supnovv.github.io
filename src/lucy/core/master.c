#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define L_LIBRARY_IMPL
#define l_buffer_ptr(b) ((l_bufhead*)b->p)
#include "core/master.h"

/**
 * thread
 */

typedef struct {
  l_smplnode node;
  l_int bsize;
} L_BUFHEAD;

typedef struct l_buffer {
  L_BUFHEAD* p;
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

extern l_rune* l_string_print_ulong(l_ulong n, l_byte* p);

static void
l_thread_initLog(l_thread* self)
{
  t->log = l_string_create(conf->log_buffer_size);

  if (l_strt_equal(l_strt_literal("stdout"), l_strt_c(conf->logfile))) {
    t->logfile.stream = stdout;
  } else if (l_strt_equal(l_strt_literal("stderr"), l_strt_c(conf->logfile))) {
    t->logfile.stream = stderr;
  } else {
    suffix = conf->prefixend;
    *suffix++ = '_';
    suffix = l_string_print_ulong(t->index, suffix);
    suffix = l_copy_n(".txt", 4, suffix);
    *suffix = 0;
    t->logfile = l_open_append_unbuffered(conf->logfile);
    l_write_strt_to_file(&t->logfile, l_strt_literal("--------" L_NEWLINE));
  }

  l_string_ptr(log)->limit = -(l_string_ptr(log)->limit); /* set log's limit to negative value */
}

static void
l_thread_init(l_thread* t, l_config* conf)
{
  l_thrblock* b = 0;
  l_byte* suffix = 0;

  if (t->block)
    return;

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

  l_thread_initLog(conf->log_buffer_size, conf->logfile);

  t->weight = t->msgwait = 0;
}

static void
l_thread_free(l_thread* t)
{
  l_smplnode* node = 0;
  l_squeue* frbq = 0;

  l_squeue msgq;
  l_squeue_init(&msgq);

  l_thread_lock(t);
  l_squeue_pushQueue(&msgq, t->rxmq);
  l_thread_unlock(t);

  l_squeue_pushQueue(&msgq, t->txmq);
  l_squeue_pushQueue(&msgq, t->txms);

  while ((node = l_squeue_pop(&msgq))) {
    l_raw_free(node);
  }

  if (t->L) {
    l_close_luastate(t->L);
    t->L = 0;
  }

  frbq = &((l_freebq*)t->frbq)->queue;
  while ((node = l_squeue_pop(frbq))) {
    l_raw_free(node);
  }

  l_mutex_free(t->svmtx);
  l_mutex_free(t->mutex);
  l_condv_free(t->condv);

  l_thread_flush_log(t);
  l_string_free(&t->log);

  if (t->logfile.stream != stdout && t->logfile.stream != stderr) {
    l_close_file(&t->logfile);
  }
}

static void*
l_thread_func(void* para)
{
  int n = 0;
  l_thread* self = (l_thread*)para;
#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = self;
#endif
  l_thrkey_setData(&l_thrkey, self);
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

static void
l_thread_exit()
{
  l_raw_thread_exit();
}

static int
l_thread_join(l_thread* self)
{
  return l_raw_thread_join(&self->id);
}

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

static l_byte*
l_buffer_start(l_buffer* buffer)
{
  return (l_byte*)(l_buffer_ptr(buffer) + 1);
}

static l_int
l_buffer_capacity(l_buffer* buffer)
{
  return l_buffer_ptr(buffer)->bsize - sizeof(l_bufsize);
}

L_PRIVAT int /* if return false, keep buffer unchanged */
l_buffer_ensureCapacity(l_buffer* buffer, l_int capacity)
{
  l_int oldsize = l_buffer_ptr(buffer)->bsize;
  void* newbuffer = 0;

  if (capacity < sizeof(L_BUFHEAD))
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
      q->frmem -= elem->bsize;

      return true;
    }

    /* else go down to alloc raw memory */
  }

  if (size < sizeof(L_BUFHEAD))
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

L_PRIVATE void
l_master_write_log(l_string* s)
{
  l_thread* thread = 0;

  if (l_string_isEmpty(s))
    return;

  thread = (l_thread*)((l_byte*)s - offsetof(l_thread, log));
  l_file_writeLen(&thread->logfile, l_string_start(s), l_string_size(s));

  l_string_clear(s);
}

L_PRIVAT void
l_master_flush_log()
{
  /* tell each thread to flush log */
}

L_PRIVAT l_string*
l_master_start_log(const l_byte* tag)
{
  l_string* log = 0;
  l_thread* thread = 0;

  if (!(thread = l_thread_self())) {
    /* printf("%s00 %s ~\n", ((char*)tag) + 2, (char*)fmt); */

    if (!(thread = l_thread_master()))
      thread = l_master_init();

    if (!thread->log.p)
      l_thread_initLog(thread);
  }

  log = &thread->log;
  l_string_format_s(log, l_strt_c(tag), 0);
  l_string_format_u(log, thread->index, L_PRECISE(2));
  l_string_format_s(log, l_strt_literal(" "), 0);
  return log;
}

static l_thread*
l_thread_acquire()
{
  l_thread* thread = (l_thread*)l_priorq_pop(&l_thread_pool);
  thread->weight += 1;
  l_priorq_push(&l_thread_pool, &thread->node);
  return thread;
}

static void
l_thread_acquireSpecific(l_thread* thread)
{
  thread->weight += 1;
  l_priorq_remove(&l_thread_pool, &thread->node);
  l_priorq_push(&l_thread_pool, &thread->node);
}

static void
l_thread_release(l_thread* thread)
{
  thread->weight -= 1;
  l_priorq_remove(&l_thread_pool, &thread->node);
  l_priorq_push(&l_thread_pool, &thread->node);
}

/**
 * service
 */

#define L_MIN_SERVICE_ID  (1024)
#define L_SERVICE_STARTED  0x01
#define L_SERVICE_CLOSING  0x02
#define L_SERVICE_STOPRX   0x04
#define L_SERVICE_SOCKET   0x08

typedef struct {
  l_smplnode node;
} l_srvcslot;

typedef struct {
  l_umedit nslot;
  l_umedit nelem;
  l_srvcslot* slot;
} l_srvctable;

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
  slot = llheadnode(self, ((l_service*)elem)->svid);
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
    if (((l_service*)srvc)->svid == svid) return (l_service*)srvc;
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
    if (((l_service*)node)->svid == svid) {
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
l_srvctable_clear(l_srvctable* self, void (*elemfree)(void*))
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
      if (elemfree) elemfree(first);
    }
  }
}

static void
l_srvctable_free(l_srvctable* self, void (*elemfree)(void*))
{
  if (!self->slot) return;
  l_srvctable_clear(self, elemfree);
  l_raw_free(self->slot);
  self->slot = 0;
  self->nelem = 0;
}

/**
 * service is single linked in the service hash table after it created.
 * service is freed as a free buffer after it is completed.
 */

typedef struct l_service {
  L_BUFHEAD HEAD;
  l_ioevent* ioev; /* guard by svmtx */
  l_ioevent event; /* guard by svmtx */
  l_uint flags; /* shared flags stop_rx_msg service is closing, guard by svmtx */
  /* thread own use */
  l_umedit flagw; /* only accessed by a worker */
  l_umedit svid; /* only set once when init, so can freely access it */
  l_thread* thread; /* only set once when init, so can freely access it */
  int (*entry)(l_service*, l_message*); /* service entry function */
  void (*destroy)(l_service*); /* destroy function for child structure's fields if necessary */
  /* coroutine */
  lua_State* co;
  int (*func)(l_service*);
  int (*kfunc)(l_service*);
  int coref;
} l_service;

L_GLOBAL l_mutex l_srvc_mtx;
L_GLOBAL l_ushort l_svid_seed; /* shared by all threads */
L_GLOBAL l_srvctable l_srvc_table; /* only accessed by master */

static void
l_master_add_service(l_service* srvc)
{
  l_srvctable_add(&l_srvc_table, &srvc->node);
}

static l_service*
l_master_find_service(l_umedit svid)
{
  return l_srvctable_find(&L_srvc_table, svid);
}

static l_service*
l_master_del_service(l_umedit svid)
{
  return l_srvctable_del(&L_srvc_table, svid);
}

static l_umedit
l_master_getRawServiceId()
{
  l_mutex* mtx = &l_srvc_mtx;
  l_umedit svid = 0;

  l_mutex_lock(mtx);
  if (++l_svid_seed < L_MIN_SERVICE_ID) {
    l_svid_seed = L_MIN_SERVICE_ID;
  }
  svid = l_svid_seed;
  l_mutex_unlock(mtx);

  return svid;
}

#define l_service_ptr(b) ((l_service*)(b->p))

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

  l_service_ptr(&buffer)->svid = l_master_getRawServiceId();
  l_service_ptr(&buffer)->thread = thread;
  l_service_ptr(&buffer)->entry = entry;
  l_service_ptr(&buffer)->destroy = destroy;
  return l_service_ptr(&buffer);
}

L_EXTERN int
l_service_free(l_service* srvc)
{
  l_buffer buffer;

  if (srvc->flagm | L_SERVICE_STARTED) {
    l_loge_s("already started");
    return false;
  }

  buffer->p = &srvc->HEAD;
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
  srvc->thread = thread;
  srvc->flagw |= L_SERVICE_STARTED;
  l_message_startService(self, srvc);
}

static l_service*
l_service_setSocket(l_service* srvc, l_filedesc sock, l_ushort masks, l_ushort flags)
{
  srvc->ioev = &srvc->event;
  srvc->flagw |= L_SERVICE_SOCKET;
  srvc->flagm |= L_SERVICE_SOCKET;

  srvc->ioev->fd = sock;
  srvc->ioev->udata = srvc->svid;
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
  l_thread* self = srvc->thread;
  l_filedesc sock = -1;

  if (srvc->flagw & L_SERVICE_SOCKET) {
    l_mutex* svmtx = thread->svmtx;

    l_mutex_lock(svmtx);
    ioev = srvc->ioev;
    /* store fd to send to mater to close */
    fd = ioev->fd;
    /* reset fd as closed */
    ioev->fd = -1;
    /* if thread still has io message in the pending queue,
    when to handle it, this message will be ignored due to
    no event attached to service from now */
    srvc->ioev = 0;
    l_mutex_unlock(svmtx);

    srvc->wflgs &= (~L_SERVICE_IO_EVENT);
  }

  if (sock == -1) return;
  l_message_closeServiceSocket(self, srvc->svid, sock);
}

/**
 * message
 */

static void
l_message_startService(l_thread* thread, l_service* srvc)
{
  l_message_sendptr(thread, L_SERVICE_MASTER_ID, L_MESSAGE_START_SERVICE, srvc);
}

static void
l_message_closeService(l_thread* thread, l_umedit svid)
{
  l_message_send(thread, L_SERVICE_MASTER_ID, L_MESSAGE_CLOSE_SERVICE, svid, -1);
}

static void
l_message_closeServiceSocket(l_thread* thread, l_umedit svid, l_filedesc sock)
{
  l_service_message* msg = (l_service_message*)l_create_message(thread, type, sizeof(l_service_message));
  msg->svid = svid;
  msg->fd = sock;
  l_send_message(thread, L_SERVICE_MASTER_ID, &msg->head);
}

static void
l_message_masterExit(l_thread* selfthread)
{
  l_message_sendtype(selfthread, L_SERVICE_MASTER_ID, L_MESSAGE_MASTER_EXIT);
}

L_GLOBAL l_squeue l_msg_rxq; /* shared by all thread */
L_GLOBAL l_mutex l_msg_mtx;

L_EXTERN void /* send message to dest service, it is may be in current thread */
l_message_send(l_thread* selfthread, l_umedit destid, l_message* msg) {
  msg->srvc = 0;
  msg->dstid = destid;

  if ((destid & 0xffff) == 0) {
    l_squeue_push(selfthread->txms, &msg->HEAD.node);
    return;
  }

  if ((destid >> 16) == selfhread->index) {
    l_thread_lock(selfthread);
    l_squeue_push(&selfthread->rxmq, &msg->HEAD.node);
    l_thread_unlock(selfthread);
    return;
  }

  l_squeue_push(selfthread->txmq, &msg->HEAD.node);
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
  l_squeue_pushQueue(outQ, master->txmq); /* messages master send to workers */
}

#define l_message_ptr(b) ((l_message*)b->p)

L_EXTERN l_message*
l_message_create(l_umedit type, l_int size, l_thread* hint) {
  l_buffer buffer;

  if (size < (l_int)sizeof(l_message)) {
    l_loge_1("size %d", ld(size));
    return 0;
  }

  if (!l_buffer_init(&buffer, size, hint)) {
    return 0;
  }

  l_message_ptr(&buffer)->type = type;
  return l_message_ptr(&buffer);
}

L_EXTERN void
l_message_free(l_message* msg, l_thread* hint) {
  l_buffer buffer = {&msg->HEAD};
  l_buffer_free(&buffer, hint);
}

L_EXTERN void
l_message_freeQueue(l_squeue* mq, l_thread* hint) {
  l_message* msg = 0;
  while ((msg = (l_message*)l_squeue_pop(mq))) {
    l_message_free(msg, hint);
  }
}

void l_send_message_tp(l_thread* thread, l_umedit destid, l_umedit type) {
  l_message* msg = l_create_message(thread, type, sizeof(l_message));
  l_send_message(thread, destid, msg);
}

void l_send_message_fd(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd) {
  l_message_fd* msg = (l_message_fd*)l_create_message(thread, type, sizeof(l_message_fd));
  msg->fd = fd;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_ptr(l_thread* thread, l_umedit destid, l_umedit type, void* ptr) {
  l_message_ptr* msg = (l_message_ptr*)l_create_message(thread, type, sizeof(l_message_ptr));
  msg->ptr = ptr;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_s32(l_thread* thread, l_umedit destid, l_umedit type, l_medit s32) {
  l_message_s32* msg = (l_message_s32*)l_create_message(thread, type, sizeof(l_message_s32));
  msg->s32 = s32;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_u32(l_thread* thread, l_umedit destid, l_umedit type, l_umedit u32) {
  l_message_u32* msg = (l_message_u32*)l_create_message(thread, type, sizeof(l_message_u32));
  msg->u32 = u32;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_s64(l_thread* thread, l_umedit destid, l_umedit type, l_long s64) {
  l_message_s64* msg = (l_message_s64*)l_create_message(thread, type, sizeof(l_message_s64));
  msg->s64 = s64;
  l_send_message(thread, destid, &msg->head);
}

void l_send_message_u64(l_thread* thread, l_umedit destid, l_umedit type, l_ulong u64) {
  l_message_u64* msg = (l_message_u64*)l_create_message(thread, type, sizeof(l_message_u64));
  msg->u64 = u64;
  l_send_message(thread, destid, &msg->head);
}

void l_send_ioevent_message(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd, l_umedit masks) {
  l_ioevent_message* msg = (l_ioevent_message*)l_create_message(thread, type, sizeof(l_ioevent_message));
  msg->fd = fd;
  msg->masks = masks;
  l_send_message(thread, destid, &msg->head);
}

void l_send_service_message(l_thread* thread, l_umedit destid, l_umedit type, l_umedit svid, l_handle fd) {
  l_service_message* msg = (l_service_message*)l_create_message(thread, type, sizeof(l_service_message));
  msg->svid = svid;
  msg->fd = fd;
  l_send_message(thread, destid, &msg->head);
}

static void
l_message_bootstrap(l_thread* thread, int (*bootstrap)())
{
  l_bootstrap_message* msg = (l_bootstrap_message*)l_create_message(thread, L_MESSAGE_BOOTSTRAP, sizeof(l_bootstrap_message));
  msg->bootstrap = bootstrap;
  l_send_message(thread, L_SERVICE_BOOTSTRAP, &msg->head);
}

/**
 * config
 */

typedef struct {
  int workers;
  int service_table_size;
  l_int log_buffer_size;
  l_int thread_max_free_memory;
  l_rune logfile[FILENAME_MAX+1];
  l_rune* prefixend;
  lua_State* L;
} l_config;

static int l_set_logfile_prefix(void* pconf, l_strt name) {
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

l_config* l_create_config() {
  l_config* conf = (l_config*)l_raw_calloc(sizeof(l_config));
  conf->L = l_new_luastate();

  conf->workers = lucy_intconf(conf->L, "workers");
  if (conf->workers < 1) {
    conf->workers = 1;
  }

  conf->log_buffer_size = lucy_intconf(conf->L, "log_buffer_size");
  if (conf->log_buffer_size < BUFSIZ) {
    conf->log_buffer_size = BUFSIZ;
  }
  else if (conf->log_buffer_size + BUFSIZ >= l_max_rdwr_size) {
    conf->log_buffer_size = l_max_rdwr_size - BUFSIZ - 1;
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
    l_set_logfile_prefix(conf, l_literal_strt("trace"));
  }

  return conf;
}

void l_free_config(l_config* self) {
  l_raw_free(self);
}


/**
 * task dispatch
 */

L_GLOBAL l_eventmgr l_eventmgr_g;
L_GLOBAL l_thread l_master_thread;
L_GLOBAL int l_num_workers;
L_GLOBAL l_thread* l_worker_thread;
L_GLOBAL l_priorq l_thread_pool;

static int
l_thread_less(void* lhs, void* rhs)
{
  return ((l_thread*)lhs)->weight < ((l_thread*)rhs)->weight;
}

static l_config*
l_master_init()
{
  int i = 0;
  l_config* conf = 0;
  l_thread* master = &l_master_thread;
  l_thread* thread = 0;

  l_thrkey_init(&l_thrkey_g);

  /* master thread */

#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = 0;
#endif
  l_thrkey_setData(&l_thrkey_g, 0);

  conf = l_create_config();
  l_thread_init(master, conf);

  master->id = l_raw_thread_self(); /* master thread created by os */
  master->L = conf->L;
  conf->L = 0;

#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = master;
#endif
  l_thrkey_setData(&l_thrkey_g, master);

  /* worker thread pool */

  l_num_workers = conf->workers;
  l_worker_thread = (l_thread*)l_raw_calloc(sizeof(l_thread) * conf->workers);

  l_priorq_init(&l_thread_pool, l_thread_less);

  for (i = 0; i < conf->workers; ++i) {
    thread = l_worker_thread + i;
    thread->index = i + 1; /* worker index should not 0 */
    l_thread_init(thread, conf);
    thread->L = l_new_luastate();
    l_priorq_push(&l_thread_pool, &thread->node);
  }

  /* socket */

  l_socket_init();
  l_eventmgr_init(&l_eventmgr_g);

  /* message */

  l_squeue_init(&l_msg_rxq);
  l_mutex_init(&l_msg_mtx);

  /* service */

  l_mutex_init(&l_srvc_mtx);
  l_svid_seed = L_MIN_SERVICE_ID;
  l_srvctable_init(&l_srvc_table, conf->service_table_size);

  return conf;
}

static void l_master_clean() {
  l_smplnode* node = 0;
  l_thread* thread = 0;

  l_thread_free(l_thread_master());
  l_eventmgr_free(&l_eventmgr_g);

  l_mutex_lock(&l_msg_mtx);
  while ((node = l_squeue_pop(&l_msg_rxq))) {
    l_raw_mfree(node);
  }
  l_mutex_unlock(&l_msg_mtx);
  l_mutex_free(&l_msg_mtx);

  l_mutex_free(&l_srvc_mtx);
  l_srvctable_free(&l_srvc_table, l_raw_mfree);

  while ((thread = (l_thread*)l_priorq_pop(&l_thread_pool))) {
    l_thread_free(thread);
    l_raw_mfree(thread->block);
  }

  l_raw_mfree(l_worker_thread);
  l_thrkey_free(&l_thrkey_g);
}

L_EXTERN l_thread*
l_thread_master()
{
  return &l_master_thread;
}


static void
l_master_wakeup()
{
  /* wakeup master to handle the message */
  l_eventmgr_wakeup(&l_eventmgr_g);
}

static void
l_message_masterHandleConnectionResponse(l_umedit dstid, l_ioevent* ev)
{
  l_thread* master = l_thread_master();
  l_message* msg = l_create_message(master, L_MESSAGE_CONNRSP, sizeof(l_ioevent_message));
  l_ioevent_message* p = (l_ioevent_message*)msg;
  p->fd = ev->fd;
  p->masks = ev->masks;
  l_send_message(master, dstid, msg);
}

static void
l_message_masterAcceptConnection(void* ud, l_sockconn* conn)
{
  l_service* sv = (l_service*)ud;
  l_thread* master = l_thread_master();
  l_message* msg = l_create_message(master, L_MESSAGE_CONNIND, sizeof(l_connind_message));
  l_connind_message* p = (l_connind_message*)msg;
  p->fd = conn->sock;
  p->remote = conn->remote;
  l_send_message(master, sv->svid, msg);
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
  if (!(srvc = l_master_find_service(rxev->udata))) return;
  if (!(thread = srvc->thread)) return;
  if (!(srvc->mflgs & L_SERVICE_IO_EVENT)) return;

  svmtx = thread->svmtx;

  l_mutex_lock(svmtx);
  srev = srvc->ioev;
  if (!srev || srev->fd != rxev->fd) {
    l_mutex_unlock(svmtx);
    return;
  }

  if (srev->flags & L_IOEVENT_FLAG_LISTEN) {
    l_mutex_unlock(svmtx);
    l_socket_accept(srev->fd, l_master_accept_connection, srvc);
    return;
  }

  if (srev->flags & L_IOEVENT_FLAG_CONNECT) {
    srev->flags &= (~L_IOEVENT_FLAG_CONNECT);
    l_mutex_unlock(svmtx);
    l_master_send_connrsp_message(srvc->svid, srev);
    return;
  }

  if (srev->masks == 0) {
    l_handle fd = srev->fd;
    l_umedit masks = srev->masks = rxev->masks;
    l_mutex_unlock(svmtx);
    l_send_ioevent_message(master, srvc->svid, L_MESSAGE_IOEVENT, fd, masks);
    return;
  }

  srev->masks |= rxev->masks;
  l_mutex_unlock(svmtx);
}

static int
l_master_handleMessage(l_squeue* frmq)
{
  l_thread* master = l_thread_master();
  l_message* msg = 0;
  l_squeue msgq;
  int exited_workers = 0;

  l_squeue_init(&msgq);

  l_thread_lock(master);
  l_squeue_push_queue(&msgq, master->rxmq);
  l_thread_unlock(master);

  l_squeue_push_queue(&msgq, master->txms);

  while ((msg = (l_message*)l_squeue_pop(&msgq))) {
    switch (msg->type) {
    case L_MESSAGE_START_SERVICE: {
        l_service* srvc = (l_service*)((l_message_ptr*)msg)->ptr;
        /* attach thread first */
        if (srvc->thread) {
          l_thread_acquireSpecific(srvc->thread);
        } else {
          srvc->thread = l_thread_acquire();
        }

        srvc->svid = (((l_umedit)srvc->thread->index) << 16) | (srvc->svid & 0xffff);

        l_logm_1("start service %d", ld(srvc->svid));

        /* then add event */
        if (srvc->ioev) {
          l_ioevent event = *srvc->ioev;
          srvc->ioev->masks = 0; /* clear masks, prepare to receive */
          l_ionfmgr_add(l_master_get_ionfmgr(), &event);
        }
        /* add service online to table */
        l_master_add_service(srvc);
        /* send confirm back */
        l_send_message_tp(master, srvc->svid, L_MESSAGE_START_SRVCRSP);
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
        l_service_message* p = (l_service_message*)msg;
        l_service* srvc = l_master_del_service(p->svid);
        l_handle fd = p->fd;

        l_logm_1("close service %d", ld(p->svid));

        if (fd != -1) {
          l_ionfmgr_del(l_master_get_ionfmgr(), fd);
          l_socket_close(fd);
        }

        if (!srvc) break;
        l_thread_release(srvc->thread); /* L_MESSAGE_CLOSE_SRVCRSP is the last msg */
        l_send_message_ptr(master, srvc->svid, L_MESSAGE_CLOSE_SRVCRSP, srvc);
      }
      break;
    case L_MESSAGE_MASTER_EXIT: {
        int i = 0;
        l_thread* t = L_worker_threads;
        l_logm_s("prepare exit");
        for (; i < L_num_workers; ++i) {
          if (t[i].index == 0) continue; /* already exit */
          l_send_message_tp(master, t[i].index, L_MESSAGE_WORKER_EXIT);
        }
      }
      break;
    case L_MESSAGE_WORKER_EXITRSP: {
        int index = ((l_message_u32*)msg)->u32;
        ++exited_workers;
        L_worker_threads[index].index = 0;
        l_logm_3("worker %d exited %d/%d", ld(index), ld(exited_workers), ld(L_num_workers));
        if (exited_workers == L_num_workers) {
          return false;
        }
      }
      break;
    default:
      l_loge_1("unknow message %d", ld(msg->type));
      break;
    }

    l_squeue_push(frmq, &msg->node);
  }

  return true;
}

static int
l_bootstrap_service_proc(l_service* self, l_message* msg)
{
  (void)self;
  switch (msg->type) {
  case L_MESSAGE_START_SRVCRSP:
    l_logm_s("bootstrap startup");
    break;
  case L_MESSAGE_BOOTSTRAP:
    l_logm_1("bootstrap service %d", ld(self->svid));
    if (((l_bootstrap_message*)msg)->bootstrap) {
      return ((l_bootstrap_message*)msg)->bootstrap();
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
  l_thread* thread = 0;
  l_thread* worker = l_worker_thread;
  l_squeue* mq = 0;
  l_uint wait_count = 0;
  int n = L_num_workers;
  int i = 0;

  l_logm_s("master run");

  mq = (l_squeue*)l_raw_calloc(sizeof(l_squeue) * n);
  for (; i < n; ++i) {
    l_squeue_init(mq + i);
  }

  l_squeue_init(&rxmq);
  l_squeue_init(&frmq);

  srvc = l_service_create(sizeof(l_service), l_bootstrap_service_proc, 0);
  srvc->svid = L_SERVICE_BOOTSTRAP;
  l_service_start(srvc); /* start bootstrap service */
  l_master_handleMessage(&frmq); /* handle bootstrap start message */
  l_message_bootstrap(master, start); /* send BOOTSTRAP message */
  l_master_wakeup(); /* wakeup first time */

  for (; ;) {
    l_logm_1("master T%d wait", ld(++wait_count));
    l_eventmgr_wait(&l_eventmgr_g, l_master_dispatchEvent);
    l_logm_1("master T%d wakeup", ld(wait_count));

    if (!l_master_handleMessages(&frmq)) {
      return 0; /* normal exit */
    }

    l_master_getMessages(master, &rxmq); /* messages need send to workers */

    while ((msg = (l_message*)l_squeue_pop(&rxmq))) {
      if (msg->type == L_MESSAGE_WORKER_EXIT) {
        msg->srvc = 0; /* msg->dstid contains thread index */
        l_squeue_push(mq + msg->dstid - 1, &msg->node);
        continue;
      }

      srvc = l_master_findService(msg->dstid);
      if (!srvc && msg->type != L_MESSAGE_CLOSE_SRVCRSP) {
        l_squeue_push(&frmq, &msg->node);
        continue;
      }

      if (msg->type == L_MESSAGE_CLOSE_SRVCRSP) {
        srvc = (l_service*)((l_message_ptr*)msg)->ptr;
        thread = srvc->thread;
      } else {
        l_mutex* svmtx = 0;
        thread = srvc->thread;
        svmtx = thread->svmtx;
        l_mutex_lock(svmtx);
        if (srvc->flags & L_SERVICE_STOPRX) {
          l_mutex_unlock(svmtx);
          l_squeue_push(&frmq, &msg->node);
          continue;
        }
        l_mutex_unlock(svmtx);
      }

      msg->srvc = srvc;
      l_squeue_push(mq + thread->index - 1, &msg->node);
    }

    for (i = 0; i < n; ++i) {
      if (l_squeue_isEmpty(mq + i)) continue;
      thread = ta + i;

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

    l_message_freeQueue(master, &frmq);
  }
}

static int
l_worker_start()
{
  l_squeue msgq, frmq;
  l_service* srvc = 0;
  l_message* msg = 0;
  l_thread* thread = 0;
  int thread_exit = 0;

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
      case L_MESSAGE_START_SRVCRSP:
        l_logm_1("service %d started", ld(srvc->svid));
        break;
      case L_MESSAGE_CLOSE_SRVCRSP:
        l_logm_1("service %d closed", ld(srvc->svid));
        l_thread_free_buffer(thread, &srvc->node);
        l_squeue_push(&frmq, &msg->node);
        continue;
      case L_MESSAGE_WORKER_EXIT:
        l_logm_1("worker %d prepare exit", ld(thread->index));
        thread_exit = 1;
        l_send_message_u32(thread, L_SERVICE_MASTER_ID, L_MESSAGE_WORKER_EXITRSP, thread->index);
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

      if (!(srvc->flagw & L_SERVICE_CLOSING)) {
        continue;
      }

      l_service_free_state(srvc);

      l_thread_lock(thread);
      srvc->flags |= L_SERVICE_STOPRX;
      l_thread_unlock(thread);

      l_service_closeSocket(srvc);
      l_message_closeService(thread, srvc->svid);
    }

    l_message_freeQueue(thread, &frmq);
    l_worker_flushMessages(thread);

    if (thread_exit) {
      break;
    }
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
  l_config* conf = 0;
  l_strt fileprefix;
  l_thread* master = 0;

  conf = l_master_init();
  master = l_thread_master();

  l_master_parse_command_line(argc, argv);

  l_logm_s("master initialized");

  fileprefix = l_strt_e(conf->logfile, conf->prefixend);
  l_logm_5("workers %d log_buffer_size %d service_table_size 2^%d thread_max_free_memory %d logfile_prefix %strt",
    ld(conf->workers), ld(conf->log_buffer_size), ld(conf->service_table_size), ld(conf->thread_max_free_memory), lstrt(&fileprefix));

  l_free_config(conf);

  for (i = 0; i < l_num_workers; ++i) {
    l_thread_start(l_worker_thread + i, l_worker_start);
  }
  l_logm_1("%d-worker started", ld(L_num_workers));

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
  l_assert(sizeof(l_mutex) >= L_MUTEX_SIZE);
  l_assert(sizeof(l_rwlock) >= L_RWLOCK_SIZE);
  l_assert(sizeof(l_condv) >= L_CONDV_SIZE);
  l_assert(sizeof(l_thrkey) >= L_THKEY_SIZE);
  l_assert(sizeof(l_thrid) >= L_THRID_SIZE);
}

