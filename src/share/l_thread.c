#include <stdlib.h>
#include "l_thread.h"

l_thrkey llg_thread_key;
l_thrkey llg_logger_key;

l_thread_local(l_thread* llg_thread_ptr);
l_thread_local(l_logger* llg_logger_ptr);

static l_priorq llg_thread_prq;

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

  l_thrkey_init(&llg_thread_key);
  l_thrkey_init(&llg_logger_key);
  l_priorq_init(&llg_thread_prq, llthreadless);

  while (numofthread-- > 0) {
    t = (l_thread*)l_raw_calloc(sizeof(l_thread));
    t->block = l_raw_malloc(sizeof(l_thrblock));
    t->index = (l_ushort)numofthread;
    l_priorq_push(&llg_thread_prq, &t->node);
  }
}

void l_threadpool_destroy() {
  l_thread* t = 0;
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

static void ll_freeq_init(l_freeq* self) {
  *self = (l_freeq){{{0},0}, 0};
  l_squeue_init(&self->queue);
}

static void ll_freeq_free(l_freeq* self, void (*elemfree)(void*)) {
  l_smplnode* node = 0;
  while ((node = l_squeue_pop(&self->queue))) {
    if (elemfree) elemfree(node);
    l_raw_free(node);
  }
}

typedef struct {
  L_COMMON_BUFHEAD
} l_bufhead;

void* l_thread_ensure_bfsize(l_freeq* q, void* buffer, l_int newsz) {
  l_int bufsz = ((l_bufhead*)buffer)->bsize;

  if (bufsz >= newsz) {
    return buffer;
  }

  if (newsz > l_max_rdwr_size) {
    l_loge_1("large %d", ld(newsz));
    return 0;
  }

  while (bufsz < newsz) {
    if (bufsz <= l_max_rdwr_size / 2) bufsz *= 2;
    else bufsz = l_max_rdwr_size;
  }

  q->ttmem += bufsz - ((l_bufhead*)buffer)->bsize;
  buffer = l_raw_realloc(buffer, ((l_bufhead*)buffer)->bsize, bufsz);
  ((l_bufhead*)buffer)->bsize = bufsz;
  return buffer;
}

void* l_thread_alloc_buffer(l_freeq* q, l_int sizeofbuffer) {
  l_smplnode* elem = 0;
  void* temp = 0;

  if (sizeofbuffer > l_max_rdwr_size) {
    l_loge_1("large %d", ld(sizeofbuffer));
    return 0;
  }

  if ((elem = l_squeue_pop(&q->queue))) {
    temp = l_thread_ensure_bfsize(q, elem, sizeofbuffer);
    q->size -= 1;
    return temp;
  }

  q->total += 1;
  q->ttmem += sizeofbuffer;
  temp = l_raw_malloc(sizeofbuffer);
  ((l_bufhead*)temp)->bsize = sizeofbuffer;
  return temp;
}

void* l_thread_pop_buffer(l_freeq* q, l_int sizeofbuffer) {
  void* p = l_thread_alloc_buffer(thread, sizeofbuffer);
  q->total -= 1;
  q->ttmem -= ((l_bufhead*)p)->bsize;
  return p;
}

void l_thread_push_buffer(l_freeq* q, void* buffer) {
  q->total += 1;
  q->ttmem += ((l_bufhead*)buffer)->bsize;
  l_thread_free_buffer(thread, buffer, 0);
}

#define L_MIN_INUSE_BUFFERS (32)

void l_freeq_free_buffer(l_freeq* q, void* buffer, void (*extrafree)(void*)) {
  l_squeue_push(&q->queue, (l_smplnode*)buffer);
  l_int inuse = q->total - q->size;
  q->size += 1;

  if (inuse < L_MIN_INUSE_BUFFERS || q->size / 4 <= inuse) {
    return;
  }

  while (q->size / 2 > inuse) {
    if (!(buffer = l_squeue_pop(&q->queue))) {
      break;
    }

    q->size -= 1;
    q->total -= 1;
    q->ttmem -= ((l_bufhead*)buffer)->bsize;

    if (extrafree) extrafree(buffer);
    l_raw_free(buffer);
  }
}

static void llthreadinit(l_thread* self) {
  l_thrblock* b = self->block;
  self->svmtx = &b->ma;
  self->mutex = &b->mb;
  self->condv = &b->ca;
  self->log = &b->l;
  self->frco = &b->co;
  self->frbf = &b->bf;
  self->rxmq = &b->qa;
  self->rxio = &b->qb;
  self->txmq = &b->qc;
  self->txms = &b->qd;

  l_mutex_init(self->svmtx);
  l_mutex_init(self->mutex);
  l_condv_init(self->condv);

  l_squeue_init(self->rxmq);
  l_squeue_init(self->rxio);
  l_squeue_init(self->txmq);
  l_squeue_init(self->txms);

  self->L = l_new_luastate();
  /* TODO: l_logger_init(self->log) */
  ll_freeq_init(self->frco);
  ll_freeq_init(self->frbf);
}

static void llthreadfree(l_thread* self) {
  if (self->svmtx) {
    l_mutex_free(self->svmtx);
    self->svmtx = 0;
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
    /* TODO: l_logger_free(self->log); */
    self->log = 0;
  }

  if (self->frco) {
    ll_freeq_free(self->frco, 0);
    self->frco = 0;
  }

  if (self->frbf) {
    ll_freeq_free(self->frbf, 0);
    self->frbf = 0;
  }

  /* TODO: free elems in rxmq/rxio/txmq/txms */
}

static void* llthreadfunc(void* para) {
  int n = 0;
  l_thread* self = (l_thread*)para;
  l_thrkey_set_data(&llg_thread_key, self);
  n = self->start();
  llthreadfree(self);
  return (void*)(l_int)n;
}

int l_thread_start(l_thread* self, int (*start)()) {
  llthreadinit(self);
  self->start = start;
  return l_raw_create_thread(&self->id, llthreadfunc, self);
}

void l_thread_exit() {
  llthreadfree(l_thread_self());
  l_raw_thread_exit();
}

void l_process_exit() {
  exit(EXIT_FAILURE);
}

int l_thread_join(l_thread* self) {
  return l_raw_thread_join(&self->id);
}

