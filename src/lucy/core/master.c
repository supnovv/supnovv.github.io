/**
 * ## service
 * services are stored in the global hash table, the table can be only accessed by the master.
 * services can be created by any thread and can be closed by the service itself:
 * a. thread allocates a service object and send L_MSGID_SRVC_START_REQ message to the master
 * b. the message carried the allocated and initialized service object
 * c. master handle the message, assign a thread to the service, and add the service to the global table
 * d. the master then send L_MSGID_SERVICE_START to the service, this is the service's first message
 * e. service can call l_service_close(srvc) to close itself, after the thread detected the service is closing
 * f. the thread set the service as L_SERVICE_STOPRX and send L_MSGID_SRVC_CLOSE_REQ to master with the service id
 * g. master handle the message, delete the service according to the id, and count down the thread's weight
 * h. the master then send L_MSGID_SERVICE_CLOSE to service, carried the removed service object
 * i. worker thread received the message, deliver it to the service and then free the service to its memory pool
 * j. L_MSGID_SERVICE_CLOSE is the service's last message, if it has extra resource to free, this is the last chance
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#define L_LIBRARY_IMPL
#define l_buffer_ptr(b) ((L_BUFHEAD*)b->p)
#define l_service_ptr(b) ((l_service*)((b)->p))
#include "core/state.h"
#include "core/queue.h"
#include "core/string.h"
#include "core/fileop.h"
#include "core/socket.h"
#include "core/thread.h"
#include "core/service.h"

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
l_set_logfile_prefix(void* pconf, l_strn name)
{
  l_config* conf = (l_config*)pconf;
  if (name.len <= 0 || name.len > FILENAME_MAX) {
    return false;
  }
  l_copy_n(name.start, name.len, conf->logfile);
  conf->logfile[name.len] = 0;
  conf->prefixend = conf->logfile + name.len;
  return true;
}

static l_config*
l_config_create()
{
  l_config* conf = (l_config*)l_raw_calloc(sizeof(l_config));
  conf->L = l_luastate_new();

  conf->workers = l_luaconf_int(conf->L, "workers");
  if (conf->workers < 0) {
    conf->workers = 0;
  }

  conf->log_buffer_size = l_luaconf_int(conf->L, "log_buffer_size");
  if (conf->log_buffer_size < BUFSIZ) {
    conf->log_buffer_size = BUFSIZ;
  }
  else if (conf->log_buffer_size + BUFSIZ >= L_MAX_RWSIZE) {
    conf->log_buffer_size = L_MAX_RWSIZE - BUFSIZ - 1;
  }
  conf->log_buffer_size = (((conf->log_buffer_size - 1) / BUFSIZ) + 1) * BUFSIZ;

  conf->service_table_size = l_luaconf_int(conf->L, "service_table_size");
  if (conf->service_table_size < 10) {
    conf->service_table_size = 10;
  }

  conf->thread_max_free_memory = l_luaconf_int(conf->L, "thread_max_free_memory");
  if (conf->thread_max_free_memory < 1024) {
    conf->thread_max_free_memory = 1024;
  }

  if (!l_luaconf_str(conf->L, l_set_logfile_prefix, conf, "logfile_prefix")) {
    /* if get from config file failed, set the default name prefix */
    l_set_logfile_prefix(conf, l_strn_literal("logcat"));
  }

  return conf;
}

static void
l_config_free(l_config* conf)
{
  if (conf->L) {
    l_luastate_close(conf->L);
    conf->L = 0;
  }

  l_raw_mfree(conf);
}

/**
 * thread
 */

typedef struct {
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

static l_thread*
l_thread_self()
{
#if defined(L_THREAD_LOCAL_SUPPORTED)
  return l_self_thread;
#else
  return (l_thread*)l_thrkey_getData(&l_thrkey_g);
#endif
}

static l_thread*
l_thread_master()
{
  return &l_master_thread;
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

L_PRIVAT l_byte* l_string_print_ulong(l_ulong n, l_byte* p);
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

static void
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
    l_luastate_close(t->L);
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

static int
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
#define L_MSGID_SRVC_START_RSP  L_MSGID_SERVICE_START
#define L_MSGID_SRVC_CLOSE_RSP  L_MSGID_SERVICE_CLOSE
#define L_MSGID_MIN_MASTER_MSG  0x10
#define L_MSGID_SRVC_START_REQ  0x11
#define L_MSGID_SRVC_CLOSE_REQ  0x12
#define L_MSGID_SRVC_DEL_EVENT  0x13
#define L_MSGID_SRVC_ADD_EVENT  0x14
#define L_MSGID_MAX_MASTER_MSG  0x80
#define L_MSGID_START_BOOTSTRAP 0x81
#define L_MSGID_MASTER_EXIT_REQ 0x08
#define L_MSGID_WORKER_EXIT_REQ 0x82
#define L_MSGID_WORKER_EXIT_RSP 0x83
#define L_MSGID_SOCK_EVENT_IND  0x84
#define L_MSGID_SOCK_CONN_RSP   0x85
#define L_MSGID_SOCK_CONN_IND   0x86
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
l_message_send_impl(l_thread* from, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64, l_message* msg)
{
  msg->dest = destid;
  msg->msgid = msgid;
  msg->data = u32;
  msg->extra = u64;

  if (from->index != 0 && l_msg_dest_tidx(msg) == from->index) {
    l_thread_lock(from);
    l_squeue_push(from->rxmq, &msg->HEAD.node);
    l_thread_unlock(from);
    return;
  }

  if ((msgid > L_MSGID_MIN_MASTER_MSG && msgid < L_MSGID_MAX_MASTER_MSG) || l_msg_dest_svid(msg) == 0) {
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
  l_message_send_impl(from, destid, msgid + L_MESSAGE_START_ID, u32, u64, msg);
}

static void
l_message_senddata_impl(l_thread* thread, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64)
{
  l_message* msg = l_message_create(sizeof(l_message), thread);
  l_message_send_impl(thread, destid, msgid, u32, u64, msg);
}

L_EXTERN void
l_message_sendData(l_thread* thread, l_ulong destid, l_umedit msgid, l_umedit u32, l_ulong u64)
{
  l_message_senddata_impl(thread, destid, msgid + L_MESSAGE_START_ID, u32, u64);
}

static void
l_message_sendtomaster_impl(l_thread* thread, l_umedit msgid, l_umedit u32, l_ulong u64)
{
  l_message_senddata_impl(thread, L_SERVICE_MASTER, msgid, u32, u64);
}

static void
l_message_startService(l_thread* thread, l_service* srvc)
{
  l_message_sendtomaster_impl(thread, L_MSGID_SRVC_START_REQ, 0, l_msg_castptr(srvc));
}

static void
l_message_closeService(l_thread* thread, l_umedit svid)
{
  l_message_sendtomaster_impl(thread, L_MSGID_SRVC_CLOSE_REQ, svid, 0);
}

static void
l_message_delServiceEvent(l_thread* thread, l_filedesc fd)
{
  l_message_sendtomaster_impl(thread, L_MSGID_SRVC_DEL_EVENT, 0, l_msg_castfd(fd));
}

static void
l_message_addServiceEvent(l_thread* thread, l_filedesc fd, l_umedit svid, l_ushort masks)
{
  l_message_senddata_impl(thread, masks, L_MSGID_SRVC_ADD_EVENT, svid, l_msg_castfd(fd));
}

static void
l_message_masterExit(l_thread* thread)
{
  l_message_sendtomaster_impl(thread, L_MSGID_MASTER_EXIT_REQ, 0, 0);
}

static void
l_message_startBootstrap(l_thread* thread, int (*bootstrap)())
{
  l_message_senddata_impl(thread, L_SERVICE_BOOTSTRAP, L_MSGID_START_BOOTSTRAP, 0, l_msg_castfunc(bootstrap));
}

/**
 * service
 *
 * service is single linked in the service hash table after it created.
 * service is freed as a free buffer after it is completed.
 */

#define l_worker_svid(thread) ((((ulong)((thread)->index))<<48) | L_SERVICE_WORKER)
#define L_SERVICE_STARTED   0x0100
#define L_SERVICE_CLOSING   0x0200
#define L_SERVICE_STOPRX    0x0400
#define L_SERVICE_SOCKET    0x0800

L_EXTERN int
l_service_initState(l_service* srvc)
{
  lua_State* L = srvc->thread->L;
  srvc->coref = LUA_NOREF;
  /**
   * lua_State* lua_newthread(lua_State* L);
   *
   * Creates a new thread, pushes it on the stack, and returns a ponter to
   * lua_State that represents this new thread. The new thread returned by
   * this function shares with the original thread its global environment,
   * but has an independent execution stack.
   *
   * There is no explicit function to close or to destroy a thread. Threads
   * are subject to garbage collection, like any Lua object.
   *
   */
  if (!(srvc->co = lua_newthread(L))) {
    l_loge_s("lua_newthread failed");
    return false;
  }
  /**
   * int luaL_ref(lua_State* L, int t);
   *
   * Creates and returns a reference, in the table at index t, for the object
   * at the top of the stack (and pops the object).
   *
   * A reference is a unique integer key. As long as you do not manually add
   * integer keys into table t, luaL_ref ensures the uniqueness of the key it
   * returns. You can retrieve an object referred by reference r by calling
   * lua_rawgeti(L, t, r). Function luaL_unref frees a reference and its
   * associated object.
   *
   * If the object at the top of the stack is nil, luaL_ref returns the
   * constant LUA_REFNIL. The constant LUA_NOREF is guaranteed to be different
   * from any reference returned by luaL_ref.
   *
   */
  srvc->coref = luaL_ref(L, LUA_REGISTRYINDEX);
  return true;
}

L_EXTERN void
l_service_freeState(l_service* srvc)
{
  if (srvc->coref == LUA_NOREF) return;
  luaL_unref(srvc->thread->L, LUA_REGISTRYINDEX, srvc->coref); /* do nothing if LUA_NOREF/LUA_REFNIL */
  srvc->co = 0;
  srvc->coref = LUA_NOREF;
}

static int /* return 0 OK, 1 YIELD, <0 L_STATUS_LUAERR or error code */
llstateresume(l_service* srvc, int nargs)
{
  /** lua_resume **
  int lua_resume(lua_State* L, lua_State* from, int nargs);
  Starts and resumes a coroutine in the given thread L.
  The parameter 'from' represents the coroutine that is resuming L.
  If there is no such coroutine, this parameter can be NULL.
  All arguments and the function value are popped from the stack when
  the function is called. The function results are pushed onto the
  stack when the function returns, and the last result is on the top
  of the stack. */
  int nelems = 0;
  int n = lua_resume(srvc->co, srvc->thread->L, nargs);
  if (n == LUA_OK) {
    /* coroutine finishes its execution without errors, the stack in L contains
    all values returned by the coroutine main function 'func'. */
    if ((nelems = lua_gettop(srvc->co)) > 0) {
      lua_Integer nerror = lua_tointeger(srvc->co, -1); /* error code is on top */
      lua_pop(srvc->co, nelems);
      l_assert(nelems == 1);
      if (nerror < 0) {
        /* user program error code */
        return (int)nerror;
      }
    }
    return L_SUCCESS;
  }

  if (n == LUA_YIELD) {
    /* coroutine yields, the stack in L contains all values passed to lua_yield. */
    if ((nelems = lua_gettop(srvc->co)) > 0) {
      lua_Integer code = lua_tointeger(srvc->co, -1); /* the code is on top */
      lua_pop(srvc->co, nelems);
      l_assert(nelems == 1);
      if (code > 0) {
        return (int)code;
      }
    }
    return L_LUAYIELD;
  }

  /* error code is returned and the stack in L contains the error object on the top.
  Lua itself only generates errors whose error object is a string, but programs may
  generate errors with any value as the error object. in case of errors, the stack
  is not unwound, so you can use the debug api over it.
  LUA_ERRRUN: a runtime error.
  LUA_ERRMEM: memory allocation error. For such errors, Lua does not call the message handler.
  LUA_ERRERR: error while running the message handler.
  LUA_ERRGCMM: error while running a __gc metamethod. For such errors, Lua does not call
  the message handler (as this kind of error typically has no relation with the function
  being called). */
  l_loge_1("lua_resume %s", ls(lua_tostring(srvc->co, -1)));
  lua_pop(srvc->co, lua_gettop(srvc->co)); /* pop elems exist including the error object */
  return L_ERRLUA;
}

static int
llstatefunc(lua_State* co)
{
  int status = 0;
  l_service* srvc = 0;
  srvc = (l_service*)lua_touserdata(co, -1);
  lua_pop(srvc->co, 1);
  status = srvc->func(srvc);
  /* never goes here if ccco->func is yield inside */
  if (status < 0) {
    lua_pushinteger(srvc->co, status);
    return 1; /* return one result */
  }
  return 0;
}

L_EXTERN int
l_service_isYield(l_service* srvc)
{
  /**
   * int lua_status(lua_State* L);
   *
   * Returns the status of the thread L. The status can be 0 (LUA_OK) for a normal thread,
   * an error code if the thread finished the execution of a lua_resume with an error, or
   * LUA_YIELD if the thread is suspended.
   *
   * You can only call functions in threads with status LUA_OK. You can resume threads with
   * status LUA_OK (to start a new coroutine) or LUA_YIELD (to resume a coroutine).
   *
   */
  return lua_status(srvc->co) == LUA_YIELD;
}

L_EXTERN int
l_service_setResume(l_service* srvc, int (*func)(l_service*))
{
  int status = 0;

  if (srvc->co == 0) {
    l_service_initState(srvc);
  }

  srvc->func = func;

  if ((status = lua_status(srvc->co)) != LUA_OK) {
    l_loge_1("lua_State is not in LUA_OK status %d", ld(status));
    return false;
  }

  return true;
}

L_EXTERN int
l_service_resume(l_service* srvc)
{
  int nargs = 0;
  int costatus = 0;

  /** int lua_status(lua_State* L) **
  LUA_OK - start a new coroutine or restart it, or can call functions
  LUA_YIELD - can resume a suspended coroutine */
  if ((costatus = lua_status(srvc->co)) == LUA_OK) {
    /* start or restart coroutine, need to provide coroutine function */
    lua_pushcfunction(srvc->co, llstatefunc);
    lua_pushlightuserdata(srvc->co, srvc);
    return llstateresume(srvc, nargs+1);
  }

  if (costatus == LUA_YIELD) {
    /* no need to provide func again when coroutine is suspended */
    return llstateresume(srvc, 0);
  }

  l_loge_s("coroutine cannot be resumed");
  return L_ERRLUA;
}

static int
llstatekfunc(lua_State* co, int status, lua_KContext ctx)
{
  l_service* srvc = (l_service*)ctx;
  l_assert(co == srvc->co); /* co passed here should be equal to srvc->co */
  (void)status; /* status always is LUA_YIELD when kfunc is called after lua_yieldk */
  status = srvc->kfunc(srvc);
  /* never goes here if srvc->func is yield inside */
  if (status < 0) {
    lua_pushinteger(co, status);
    return 1; /* return one result */
  }
  return 0;
}

L_EXTERN int
l_service_yieldWith(l_service* srvc, int (*kfunc)(l_service*), int code)
{
  int status = 0;
  int nresults = 0;
  srvc->kfunc = kfunc;
  /* int lua_yieldk(lua_State* co, int nresults, lua_KContext ctx, lua_KFunction k);
  Usually, this function does not return; when the coroutine eventually resumes, it
  continues executing the continuation function. However, there is one special case,
  which is when this function is called from inside a line or a count hook. In that
  case, lua_yielkd should be called with no continuation (probably in the form of
  lua_yield) and no results, and the hook should return immediately after the call.
  Lua will yield and, when the coroutine resumes again, it will continue the normal
  execution of the (Lua) function that triggered the hook.
  This function can raise an error if it is called from a thread with a pending C
  call with no continuation function, or it is called from a thread that is not
  running inside a resume (e.g., the main thread). */
  if (code > 0) {
    lua_pushinteger(srvc->co, code);
    nresults += 1;
  }
  status = lua_yieldk(srvc->co, nresults, (lua_KContext)srvc, llstatekfunc);
  l_loge_s("lua_yieldk never returns to here"); /* the code never goes here */
  return status;
}

L_EXTERN int
l_service_yield(l_service* srvc, int (*kfunc)(l_service*))
{
  return l_service_yieldWith(srvc, kfunc, 0);
}

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
  return (l_umedit)(srvc->svid & 0xffffffff);
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

typedef struct {
  l_service* srvc;
  l_srvcslot* hint;
} l_frontsrvc;

static l_frontsrvc
l_srvctable_delFront(l_srvctable* self, const l_frontsrvc* hint)
{
  l_srvcslot* start = (hint && hint->hint) ? hint->hint : self->slot;
  l_srvcslot* end = self->slot + self->nslot;
  l_smplnode* elem = 0;
  for (; start < end; ++start) {
    elem = start->node.next;
    if (elem) {
      start->node.next = elem->next;
      return (l_frontsrvc){(l_service*)elem, start};
    }
  }
  return (l_frontsrvc){0, 0};
}


static void
l_srvctable_foreach(l_srvctable* self, void (*cb)(l_service*))
{
  l_srvcslot* slot = self->slot;
  l_srvcslot* end = slot + self->nslot;
  l_smplnode* elem = 0;
  for (; slot < end; ++slot) {
    elem = slot->node.next;
    while (elem) {
      cb((l_service*)elem);
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

static l_frontsrvc
l_master_delFrontService(const l_frontsrvc* hint)
{
  return l_srvctable_delFront(&l_srvc_table, hint);
}

static l_umedit
l_master_new_svid()
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
l_service_createFrom(l_service* from, l_int size, int (*entry)(l_service*, l_message*))
{
  l_buffer buffer;
  l_thread* thread = 0;

  if (size < (l_int)sizeof(l_service)) {
    l_loge_1("size %d", ld(size));
    return 0;
  }

  if (from && from->thread) {
    thread = from->thread;
  } else {
    thread = l_thread_self();
  }

  if (!l_buffer_init(&buffer, size, thread)) {
    return 0;
  }

  l_service_ptr(&buffer)->evfd = l_filedesc_empty();
  l_service_ptr(&buffer)->svid = l_master_new_svid();
  l_service_ptr(&buffer)->thread = thread;
  l_service_ptr(&buffer)->entry = entry;
  return l_service_ptr(&buffer);
}

L_EXTERN l_service*
l_service_create(l_int size, int (*entry)(l_service*, l_message*))
{
  return l_service_createFrom(0, size, entry);
}

L_EXTERN void
l_service_close(l_service* srvc)
{
  if (srvc->flagw | L_SERVICE_STARTED) {
    srvc->flagw |= L_SERVICE_CLOSING;
  } else {
    l_buffer buffer = {srvc};
    l_buffer_free(&buffer, l_thread_self());
  }
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
l_service_set_event_impl(l_service* srvc, l_filedesc fd, l_ushort masks, l_ushort flags)
{
  srvc->evfd = fd;
  srvc->evmk = masks;
  srvc->flags = flags;
  srvc->flagw |= L_SERVICE_SOCKET;
  return srvc;
}

static l_service*
l_service_setEventImpl(l_service* srvc, l_filedesc fd, l_ushort masks, l_ushort flags)
{
  if (srvc->flagw | L_SERVICE_STARTED) {
    l_loge_1("already started %d", ld(srvc->svid));
    return srvc;
  }

  if (l_filedesc_isEmpty(fd)) {
    return srvc;
  }

  return l_service_set_event_impl(srvc, fd, masks, flags);
}

L_EXTERN l_service*
l_service_setEvent(l_service* srvc, l_filedesc fd, l_ushort masks)
{
  return l_service_setEventImpl(srvc, fd, masks, 0);
}

L_EXTERN l_service*
l_service_setListen(l_service* srvc, l_filedesc fd)
{
  return l_service_setEventImpl(srvc, fd, L_SOCKET_READ, L_SOCKET_FLAG_LISTEN);
}

L_EXTERN l_service*
l_service_setConnect(l_service* srvc, l_filedesc fd)
{
  return l_service_setEventImpl(srvc, fd, L_SOCKET_RDWR, L_SOCKET_FLAG_CONNECT);
}

L_EXTERN void
l_service_delEvent(l_service* srvc)
{
  l_thread* thread = srvc->thread;
  l_filedesc fd = l_filedesc_empty();

  if (!(srvc->flagw | L_SERVICE_STARTED)) {
    l_loge_1("service not started %d", ld(srvc->svid));
    return;
  }

  if (srvc->flagw & L_SERVICE_SOCKET) {
    l_mutex* svmtx = thread->svmtx;

    l_mutex_lock(svmtx);
    fd = srvc->evfd;
    srvc->evfd = l_filedesc_empty();
    l_mutex_unlock(svmtx);

    srvc->flagw &= (~L_SERVICE_SOCKET);
  }

  if (l_filedesc_isEmpty(fd)) {
    return;
  }

  l_message_delServiceEvent(thread, fd);
}

L_EXTERN void
l_service_mod_event_impl(l_service* srvc, l_filedesc fd, l_ushort masks, l_ushort flags)
{
  l_filedesc oldfd;
  l_thread* thread = 0;
  l_mutex* svmtx = 0;

  if (!(srvc->flagw | L_SERVICE_STARTED)) {
    l_loge_1("service not started %d", ld(srvc->svid));
    return;
  }

  oldfd = l_filedesc_empty();
  thread = srvc->thread;
  svmtx = thread->svmtx;

  l_mutex_lock(svmtx);
  if (!l_filedesc_isEmpty(srvc->evfd)) {
    oldfd = srvc->evfd;
    srvc->evfd = l_filedesc_empty();
  }
  srvc->flagw &= (~L_SERVICE_SOCKET);
  if (!l_filedesc_isEmpty(fd)) {
    l_service_set_event_impl(srvc, fd, masks, flags);
  }
  srvc->evmk = 0; /* clear masks, prepare to receive */
  l_mutex_unlock(svmtx);

  if (!l_filedesc_isEmpty(oldfd)) {
    l_message_delServiceEvent(thread, fd);
  }

  if (!l_filedesc_isEmpty(fd)) {
    l_message_addServiceEvent(srvc->thread, fd, l_service_id_for_lookup(srvc), masks);
  }
}

L_EXTERN void
l_service_modEvent(l_service* srvc, l_filedesc fd, l_ushort masks)
{
  l_service_mod_event_impl(srvc, fd, masks, 0);
}

L_EXTERN void
l_service_modListen(l_service* srvc, l_filedesc fd)
{
  l_service_mod_event_impl(srvc, fd, L_SOCKET_READ, L_SOCKET_FLAG_LISTEN);
}

L_EXTERN void
l_service_modConnect(l_service* srvc, l_filedesc fd)
{
  l_service_mod_event_impl(srvc, fd, L_SOCKET_RDWR, L_SOCKET_FLAG_CONNECT);
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

  /* master thread */

#if defined(L_THREAD_LOCAL_SUPPORTED)
  l_self_thread = 0;
#endif
  l_thrkey_setData(&l_thrkey_g, 0);

  conf = l_config_create();
  prefix = l_strt_from(conf->logfile, conf->prefixend);

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
      thread->L = l_luastate_new();
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

  l_initialized = true;

  /* others */

  l_logm_5("workers %d log_buffer_size %d service_table_size 2^%d thread_max_free_memory %d logfile_prefix %strt",
      ld(conf->workers), ld(conf->log_buffer_size), ld(conf->service_table_size), ld(conf->thread_max_free_memory), lstrt(&prefix));

  l_config_free(conf);
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

static void
l_master_acceptConnection(void* ud, l_sockconn* conn)
{
  l_thread* master = l_thread_master();
  l_service* srvc = (l_service*)ud;
  l_sockaddr* rmt = &conn->remote;
  l_umedit family = l_sockaddr_family(rmt);
  l_message* msg = l_message_create(sizeof(l_connind_message), master);
  l_sockaddr_ip(rmt, ((l_connind_message*)msg)->addr, 16);
  l_message_send_impl(master, l_service_id(srvc), L_MSGID_SOCK_CONN_IND, (family << 16) | l_sockaddr_port(rmt), l_msg_castfd(conn->sock), msg);
}

static void
l_master_dispatchEvent(l_ioevent* rxev)
{
  l_thread* master = l_thread_master();
  l_service* srvc = 0;
  l_thread* thread = 0;
  l_mutex* svmtx = 0;

  if (l_filedesc_isEmpty(rxev->fd) || rxev->masks == 0) return;
  if (!(srvc = l_master_findService(rxev->udata))) return;

  thread = srvc->thread;
  svmtx = thread->svmtx;

  l_mutex_lock(svmtx);
  if (!l_filedesc_equal(rxev->fd, srvc->evfd)) {
    l_mutex_unlock(svmtx);
    return;
  }

  if (srvc->flags & L_SOCKET_FLAG_LISTEN) {
    l_mutex_unlock(svmtx);
    /* TODO: error check */
    l_socket_accept(rxev->fd, l_master_acceptConnection, srvc);
    return;
  }

  if (srvc->flags & L_SOCKET_FLAG_CONNECT) {
    srvc->flags &= (~L_SOCKET_FLAG_CONNECT);
    l_mutex_unlock(svmtx);
    /* TODO: error check and what if current the data from remote can be read */
    l_message_senddata_impl(master, l_service_id(srvc), L_MSGID_SOCK_CONN_RSP, rxev->masks, l_msg_castfd(rxev->fd));
    return;
  }

  if (srvc->evmk == 0) {
    srvc->evmk = rxev->masks;
    l_mutex_unlock(svmtx);
    l_message_senddata_impl(master, l_service_id(srvc), L_MSGID_SOCK_EVENT_IND, rxev->masks, l_msg_castfd(rxev->fd));
    return;
  }

  srvc->evmk |= rxev->masks;
  l_mutex_unlock(svmtx);
}

static int
l_worker_handleMessage(l_thread* thread, l_message* msg)
{
  l_buffer buffer;
  l_service* srvc = 0;
  l_mutex* mtx = 0;

  if (msg->dest == L_SERVICE_WORKER) {
    switch (msg->msgid) {
    case L_MSGID_SRVC_CLOSE_RSP: /* master already remove the service out of the table */
      srvc = (l_service*)l_msg_getptr(msg);
      srvc->entry(srvc, msg); /* let service handle the last one msg L_MSGID_SRVC_CLOSE_RSP */
      l_logm_1("service %d closed", ld(srvc->svid));
      buffer.p = srvc;
      l_buffer_free(&buffer, thread);
      return true;
    case L_MSGID_WORKER_EXIT_REQ:
      l_logm_1("worker %d prepare exit", ld(thread->index));
      l_message_senddata_impl(thread, L_SERVICE_MASTER, L_MSGID_WORKER_EXIT_RSP, thread->index, 0);
      return false; /* worker exit */
    default:
      break;
    }
    l_loge_1("unknown worker message %d", ld(msg->msgid));
    return true;
  }

  srvc = (l_service*)msg->dest;
  if (srvc->flagw & L_SERVICE_CLOSING) {
    return true;
  }

  switch (msg->msgid) {
  case L_MSGID_SOCK_EVENT_IND:
    mtx = thread->svmtx;
    l_mutex_lock(mtx);
    msg->data = srvc->evmk;
    srvc->evmk = 0;
    l_mutex_unlock(mtx);
    break;
  case L_MSGID_SRVC_START_RSP:
    l_logm_1("service %d started", ld(srvc->svid));
    break;
  default:
    break;
  }

  srvc->entry(srvc, msg);

  if (srvc->flagw & L_SERVICE_CLOSING) {
    l_service_freeState(srvc);

    l_thread_lock(thread);
    srvc->flags |= L_SERVICE_STOPRX;
    l_thread_unlock(thread);

    l_service_delEvent(srvc);
    l_message_closeService(thread, l_service_id_for_lookup(srvc));
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
        l_service* srvc = (l_service*)l_msg_getptr(msg);

        /* attach thread first */
        if (srvc->thread) {
          l_thread_acquireSpecific(srvc->thread);
        } else {
          srvc->thread = l_thread_acquire();
        }

        if (!srvc->thread) { /* if no worker threads, just use master thread */
          srvc->thread = l_thread_master();
        }

        srvc->svid = (((l_ulong)srvc->thread->index) << 48) | l_service_id_for_lookup(srvc);

        l_logm_1("start service %d", ld(srvc->svid));

        /* then add event, no need to lock mutex before started */
        if (!l_filedesc_isEmpty(srvc->evfd)) {
          l_ioevent event;
          event.fd = srvc->evfd;
          event.udata = l_service_id_for_lookup(srvc);
          event.masks = srvc->evmk;
          event.flags = 0;
          srvc->evmk = 0; /* clear masks, prepare to receive */
          l_eventmgr_add(&l_eventmgr_g, &event);
        }

        /* add service online to table */
        l_master_addService(srvc);

        /* send confirm back */
        l_message_senddata_impl(master, l_service_id(srvc), L_MSGID_SRVC_START_RSP, 0, 0);
      }
      break;
    case L_MSGID_SRVC_DEL_EVENT: {
        l_filedesc fd = l_msg_getfd(msg);
        l_eventmgr_del(&l_eventmgr_g, fd);
        l_filedesc_close(&fd);
      }
      break;
    case L_MSGID_SRVC_ADD_EVENT: {
        l_ioevent event;
        event.fd = l_msg_getfd(msg);
        event.udata = msg->data; /* svid */
        event.masks = (l_ushort)(msg->dest & 0xffff); /* l_ushort masks */
        event.flags = 0;
        l_eventmgr_add(&l_eventmgr_g, &event);
      }
      break;
    case L_MSGID_SRVC_CLOSE_REQ: {
        l_service* srvc = l_master_delService(msg->data);
        l_logm_1("close service %d", ld(msg->data));
        if (!srvc) break;
        l_thread_release(srvc->thread); /* count down thread's weight, and L_MSGID_SRVC_CLOSE_RSP is service's last msg */
        l_message_senddata_impl(master, l_worker_svid(srvc->thread), L_MSGID_SRVC_CLOSE_RSP, 0, l_msg_castptr(srvc));
      }
      break;
    case L_MSGID_MASTER_EXIT_REQ: {
        int i = 0;
        l_frontsrvc front = {0, 0};
        l_thread* thread = l_worker_thread;
        l_logm_s("prepare exit");

        while (((front = l_master_delFrontService(&front)), front.srvc)) { /* close all services first */
          thread = front.srvc->thread;
          l_thread_release(thread); /* count down thread's weight */
          l_message_senddata_impl(master, l_worker_svid(thread), L_MSGID_SRVC_CLOSE_RSP, 0, l_msg_castptr(front.srvc));
          /* TODO: if services are too many and there is not enough memory to alloc messages, consider to send multiple times */
        }

        if (front.srvc) { /* the services are not close completed */
          l_master_addService(front.srvc);
          l_thread_acquireSpecific(front.srvc->thread);
          l_message_sendtomaster_impl(master, L_MSGID_MASTER_EXIT_REQ, 0, 0);
          break;
        }

        if (!l_worker_thread || l_num_workers <= 0) { /* no workers, exit directly */
          l_message_sendtomaster_impl(master, L_MSGID_WORKER_EXIT_RSP, 0, 0);
          break;
        }

        for (i = 0; i < l_num_workers; ++i) {
          if (l_worker_thread[i].index == 0) continue; /* already exit */
          l_message_senddata_impl(master, l_worker_svid(l_worker_thread+i), L_MSGID_WORKER_EXIT_REQ, 0, 0);
        }
      }
      break;
    case L_MSGID_WORKER_EXIT_RSP:
      if (l_worker_thread) {
        int index = msg->data;
        l_worker_thread[index-1].index = 0;
        ++exited_workers;
        l_logm_3("worker %d exited %d/%d", ld(index), ld(exited_workers), ld(l_num_workers));
        if (exited_workers == l_num_workers) {
          masterExit = true;
        }
      } else {
        masterExit = true;
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
  case L_MSGID_SERVICE_START:
    l_logm_s("bootstrap started");
    break;
  case L_MSGID_SERVICE_CLOSE:
    l_logm_s("bootstrap closed");
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

L_EXTERN void
l_master_exit()
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
  l_umedit destsvid = 0;
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

  srvc = l_service_create(sizeof(l_service), l_bootstrap_service_proc);
  srvc->svid = L_SERVICE_BOOTSTRAP;
  l_service_start(srvc); /* start bootstrap service */
  l_master_handleMessage(&frmq); /* handle bootstrap start message */
  l_message_startBootstrap(master, start); /* send BOOTSTRAP message */

  for (; ;) {
    if (l_squeue_isEmpty(master->txms) && l_squeue_isEmpty(master->txmq)) {
      l_logm_1("master T%d wait", ld(++waitCount));
      l_eventmgr_wait(&l_eventmgr_g, l_master_dispatchEvent);
      l_logm_1("master T%d wakeup", ld(waitCount));
    }

    if (!l_master_handleMessage(&frmq)) {
      l_message_freeQueue(&frmq, master);
      exitCode = 0; /* normal exit */
      break;
    }

    l_master_getMessages(master, &rxmq); /* messages need send to workers */

    while ((msg = (l_message*)l_squeue_pop(&rxmq))) {
      destsvid = l_msg_dest_svid(msg);

      if (destsvid == L_SERVICE_WORKER) {
        l_uint index = l_msg_dest_tidx(msg);
        srvc = (l_service*)(l_uint)L_SERVICE_WORKER;
        if (index == 0) {
          if (l_worker_thread) { /* already exit */
            l_squeue_push(&frmq, &msg->HEAD.node);
            l_loge_1("worker service message %d cannot handle", ld(msg->msgid));
            continue;
          }
          thread = l_thread_master();
        } else {
          thread = worker + index - 1;
        }
      } else {
        srvc = l_master_findService(destsvid);
        if (!srvc) {
          l_squeue_push(&frmq, &msg->HEAD.node);
          continue;
        }
        thread = srvc->thread;
        l_mutex_lock(thread->svmtx);
        if (srvc->flags & L_SERVICE_STOPRX) { /* if this service is stopped to receive message */
          l_mutex_unlock(thread->svmtx);
          l_squeue_push(&frmq, &msg->HEAD.node);
          continue;
        }
        l_mutex_unlock(thread->svmtx);
      }

      msg->dest = (l_ulong)(l_uint)srvc;

      if (thread == l_thread_master()) {
        l_squeue_push(master->txms, &msg->HEAD.node);
        continue;
      }

      if (thread->index == 0) { /* already exit */
        l_squeue_pushQueue(&frmq, mq + i);
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
    l_worker_flushMessages(thread);

    if (threadExit) {
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
  return 0;
}

static int
llstatetestfunc(l_service* srvc)
{
  static int i = 0;
  switch (i) {
  case 0:
    ++i;
    return l_service_yield(srvc, llstatetestfunc);
  case 1:
    ++i;
    return l_service_yieldWith(srvc, llstatetestfunc, 3);
  case 2:
    ++i;
    return l_service_yield(srvc, llstatetestfunc);
  case -1:
    --i;
    return l_service_yieldWith(srvc, llstatetestfunc, 5);
  case -2:
    --i;
    return l_service_yield(srvc, llstatetestfunc);
  case -3:
    i = 0;
    return -3;
  default:
    i = -1;
    break;
  }
  return 0;
}

static int
llstatenoyield(l_service* srvc)
{
  static int i = 0;
  (void)srvc;
  switch (i) {
  case 0:
    ++i;
    return 0;
  case 1:
    ++i;
    return -2;
  case 2:
    ++i;
    return -3;
  default:
    i = 0;
    break;
  }
  return 0;
}

static void
l_resume_test()
{
  lua_State* L = l_luastate_new();
  l_service srvc;
  l_thread thread;
  srvc.thread = &thread;
  thread.L = L;
  l_service_initState(&srvc);

  l_service_setResume(&srvc, llstatetestfunc);
  l_assert(l_service_resume(&srvc) == L_LUAYIELD);
  l_assert(l_service_resume(&srvc) == 3); /* yield */
  l_assert(l_service_resume(&srvc) == L_LUAYIELD);
  l_assert(l_service_resume(&srvc) == 0);
  l_assert(l_service_resume(&srvc) == 5); /* yield */
  l_assert(l_service_resume(&srvc) == L_LUAYIELD);
  l_assert(l_service_resume(&srvc) == -3);

  l_service_setResume(&srvc, llstatenoyield);
  l_assert(l_service_resume(&srvc) == 0);
  l_assert(l_service_resume(&srvc) == -2);
  l_assert(l_service_resume(&srvc) == -3);
  l_assert(l_service_resume(&srvc) == 0);

  l_assert(sizeof(lua_KContext) >= sizeof(void*));

  l_assert(l_luaconf_int(L, "test.a") == 10);
  l_assert(l_luaconf_int(L, "test.t.b") == 20);
  l_assert(l_luaconf_int(L, "test.t.c") == 30);
  l_assert(l_luaconf_int(L, "test.d") == 40);

  /* srvc->co shared with L's global environment */
  l_assert(l_luaconf_int(srvc.co, "test.a") == 10);
  l_assert(l_luaconf_int(srvc.co, "test.t.b") == 20);
  l_assert(l_luaconf_int(srvc.co, "test.t.c") == 30);
  l_assert(l_luaconf_int(srvc.co, "test.d") == 40);

  l_service_freeState(&srvc);
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
  l_resume_test();
}

