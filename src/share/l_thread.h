#ifndef l_thread_lib_h
#define l_thread_lib_h
#include "thatcore.h"

typedef struct lua_State lua_State;
typedef struct l_service l_service;

#define L_COMMON_BUFHEAD \
  l_smplnode node; \
  l_int bsize;

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
  l_logger* log;
  l_thrid id;
  int (*start)();
  void* frbfq;
  void* block;
} l_thread;

l_extern l_thrkey llg_thread_key;
l_extern_thread_local(l_thread* llg_thread_ptr);

l_extern l_thread* l_thread_master();
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

l_extern void* l_thread_alloc_buffer(l_thread* self, l_int sizeofbuffer);
l_extern void* l_thread_ensure_bfsize(l_thread* self, l_smplnode* buffer, l_int size);
l_extern void l_thread_free_buffer(l_thread* self, l_smplnode* buffer);
l_extern void* l_thread_acquire_buffer(l_thread* self, l_int sizeofbuffer);
l_extern void l_thread_release_buffer(l_thread* self, l_smplnode* buffer);

l_extern int l_thread_start(l_thread* self, int (*start)());
l_extern int l_thread_join(l_thread* self);
l_extern void l_thread_exit();
l_extern void l_process_exit();

#endif /* l_thread_lib_h */

