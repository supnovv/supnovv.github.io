#ifndef l_thread_lib_h
#define l_thread_lib_h
#include "thatcore.h"

typedef struct lua_State lua_State;
typedef struct l_service l_service;

#define L_COMMON_BUFHEAD \
  l_smplnode node; \
  l_int bsize;

typedef struct {
  l_squeue queue; /* free buffer q */
  l_int size;  /* size of the queue */
  l_int total; /* total newed elems */
  l_int ttmem; /* total memory size */
  l_int limit; /* memory size limit */
} l_freeq;

typedef struct l_thread {
  l_linknode node;
  l_umedit weight;
  l_ushort index;
  int msgwait;
  /* shared with master */
  l_mutex* svmtx; /* guard service->ioev */
  l_mutex* mutex;
  l_condv* condv;
  l_squeue* rxmq;
  /* thread own use */
  lua_State* L;
  l_squeue* txmq;
  l_squeue* txms;
  l_logger* log;
  l_freeq* frbf;
  l_thrid id;
  int (*start)();
  void* block;
} l_thread;

l_extern l_thrkey llg_thread_key;
l_extern_thread_local(l_thread* llg_thread_ptr);

l_inline l_thread* l_thread_self() {
#if defined(l_thread_local_supported)
  return llg_thread_ptr;
#else
  return (l_thread*)l_thrkey_get_data(&llg_thread_key);
#endif
}

l_extern l_thrkey llg_logger_key;
l_extern_thread_local(l_logger* llg_logger_ptr);

l_inline l_logger* l_thread_logger() {
#if defined(l_thread_local_supported)
  return llg_logger_ptr;
#else
  return (l_thread*)l_thrkey_get_data(&llg_logger_key);
#endif
}

l_inline void l_thread_lock(l_thread* self) {
  l_mutex_lock(self->mutex);
}

l_inline void l_thread_unlock(l_thread* self) {
  l_mutex_unlock(self->mutex);
}

l_extern void l_threadpool_create(int numofthread);
l_extern void l_threadpool_destroy();
l_extern l_thread* l_threadpool_acquire();
l_extern void l_threadpool_release(l_thread* t);

l_extern void* l_thread_alloc_buffer(l_freeq* q, l_int sizeofbuffer);
l_extern void* l_thread_ensure_bfsize(l_freeq* q, void* buffer, l_int newsz);
l_extern void l_thread_free_buffer(l_freeq* q, void* buffer, void (*extrafree)(void*));
l_extern void* l_thread_pop_buffer(l_freeq* q, l_int sizeofbuffer);
l_extern void l_thread_push_buffer(l_freeq* q, void* buffer);

l_extern l_thread* l_master_thread();
l_extern int l_thread_start(l_thread* self, int (*start)());
l_extern int l_thread_join(l_thread* self);
l_extern void l_thread_exit();
l_extern void l_process_exit();

#endif /* l_thread_lib_h */

