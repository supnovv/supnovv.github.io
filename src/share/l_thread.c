#include "thatcore.h"

l_thrkey llg_thread_key;
l_thrkey llg_logger_key;

l_thread* llg_thread_ptr;
l_logger* llg_logger_ptr;

static l_priorq llg_thread_prq;

typedef struct {
  l_mutex ma;
  l_mutex mb;
  l_condv ca;
  l_logger l;
  l_freeq co;
  l_freeq bf;
} l_thrblock;

static int llthreadless(void* elem, void* elem2) {
  return ((l_thread*)thread)->weight < ((l_thread*)elem2)->weight;
}

void l_threadpool_create(int numofthread) {
  l_thread* t = 0;

  l_thrkey_init(&llg_thread_key);
  l_thrkey_init(&llg_logger_key);
  l_prioriq_init(&llg_thread_prq, llthreadless);

  while (numofthread-- > 0) {
    t = (l_thread*)l_raw_calloc(sizeof(l_thread));
    t->thrblock = l_raw_malloc(sizeof(l_thrblk));
    t->index = (l_ushort)numofthread;
    l_priorq_push(&llg_thread_prq, &t->node);
  }
}

void l_threadpool_destroy() {
  l_thread* t = 0;
  while ((t = (l_thread*)l_priorq_pop(&llg_thread_prq))) {
    l_raw_free(t->tb);
    l_raw_free(t);
  }
}

l_thread* l_threadpool_acquire() {
  l_thread* thread = (l_thread*)ccpriorq_pop(&llg_thread_prq);
  thread->weight += 1;
  l_priorq_push(&llg_thread_prq, &thread->node);
  return thread;
}

void l_threadpool_release(l_thread* thread) {
  thread->weight -= 1;
  l_priorq_remove(&llg_thread_prq, &thread->node);
  l_priorq_push(&llg_thread_prq, &thread->node);
}

static void llfreeqinit(l_freeq* self) {
  *self = (l_freeq){{{0},0}, 0};
  l_squeue_init(&self->queue);
}

static void llfreeqfree(l_freeq* self, void (*elemfree)(void*)) {
  l_smplnode* node = 0;
  while ((node = l_squeue_pop(&self->queue))) {
    if (elemfree) elemfree(node);
    l_raw_free(node);
  }
}

typedef struct {
  L_COMMON_BUFHEAD
} l_bufhead;

void* l_thread_ensure_bfsize(l_thread* thread, void* buffer, l_int newsz) {
  l_int bufsz = ((l_bufhead*)buffer)->bsize;
  l_freeq* q = thread->frbf;

  if (bufsz >= newsz) {
    return buffer;
  }

  if (newsz > l_max_rdwr_size) {
    l_loge_1("large %d", ld(newsz));
    return 0;
  }

  while (bufsz < newsz) {
    if (bufsz <= l_max_rdwr_size / 2) sz *= 2;
    else bufsz = l_max_rdwr_size;
  }

  q->ttmem += bufsz - ((l_bufhead*)buffer)->bsize;
  buffer = l_raw_realloc(buffer, ((l_bufhead*)buffer)->bsize, bufsz);
  ((l_bufhead*)buffer)->bsize = bufsz;
  return buffer;
}

void* l_thread_alloc_buffer(l_thread* thread, l_int sizeofbuffer) {
  l_smplnode* elem = 0;
  l_freeq* q = thread->frbf;
  void* temp = 0;

  if ((elem = l_squeue_pop(&q->queue))) {
    temp = l_thread_ensure_bfsize(q, elem, sizeofbuffer);

    if (!temp) {
      l_squeue_push(&q->queue, elem);
      return 0;
    }

    q->size -= 1;
    return temp;
  }

  q->total += 1;
  q->ttmem += sizeofbuffer;
  temp = l_raw_malloc(sizeofbuffer);
  ((l_bufhead*)temp)->bsize = sizeofbuffer;
  return temp;
}

void* l_thread_pop_buffer(l_thread* thread, l_int sizeofbuffer) {
  l_freeq* q = thread->frbf;
  void* p = l_thread_alloc_buffer(thread, sizeofbuffer);
  q->total -= 1;
  q->ttmem -= ((l_bufhead*)p)->bsize;
  return p;
}

void l_thread_push_buffer(l_thread* thread, void* buffer) {
  l_freeq* q = thread->frbf;
  q->total += 1;
  q->ttmem += ((l_bufhead*)p)->bsize;
  l_thread_free_buffer(thread, buffer, 0);
}

void l_freeq_free_buffer(l_thread* thread, void* buffer, void (*extrafree)(void*)) {
  l_freeq* q = thread->frbf;

  l_squeue_push(&q->queue, (l_smplnode*)buffer);
  q->size += 1;

  if (q->size / 4 <= q->total) {
    return;
  }

  while (q->size / 2 > max) {
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
  l_thrblock* b = self->thrblock;
  self->elock = &b->ma;
  self->mutext = &b->mb;
  self->condv = &b->ca;
  self->log = &b->l;
  self->frco = &b->co;
  self->frbf = &b->bf;

  l_mutex_init(self->elock);
  l_mutex_init(self->mutex);
  l_condv_init(self->condv);

  l_dqueue_init(&self->workrxq);
  l_dqueue_init(&self->workq);
  l_squeue_init(&self->msgq);

  self->L = l_lua_newstate();
  l_logger_init(self->log);
  l_freeq_init(self->frco);
  l_freeq_init(self->frbf);
}

static void llthreadfree(l_thread* self) {
  if (self->elock) {
    l_mutex_free(self->elock);
    self->elock = 0;
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
    l_logger_free(self->log);
    self->log = 0;
  }

  if (self->frco) {
    l_freeq_free(self->frco);
    self->frco = 0;
  }

  if (self->frbf) {
    l_freeq_free(self->frbf, 0);
    self->frbf = 0;
  }

  /* TODO: free elems in workrxq/workq/msgq */
}

static void* llthreadfunc(void* para) {
  int n = 0;
  l_thread* self = (l_thread*)para;
  l_thrkey_set_data(&llg_thread_key, self);
  n = self->start();
  llthreadfree(self);
  return (void*)(l_intptr)n;
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

