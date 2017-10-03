#ifndef lucy_core_thread_h
#define lucy_core_thread_h
#include "core/base.h"

typedef struct {
  L_PLAT_IMPL_SIZE(L_MUTEX_SIZE);
} l_mutex;

typedef struct {
  L_PLAT_IMPL_SIZE(L_RWLOCK_SIZE);
} l_rwlock;

typedef struct {
  L_PLAT_IMPL_SIZE(L_CONDV_SIZE);
} l_condv;

typedef struct {
  L_PLAT_IMPL_SIZE(L_THRID_SIZE);
} l_thrid;

typedef struct {
  L_PLAT_IMPL_SIZE(L_THKEY_SIZE);
} l_thrkey;

l_spec_specif(void)
l_thrkey_init(l_thrkey* self);

l_spec_specif(void)
l_thrkey_free(l_thrkey* self);

l_spec_specif(void)
l_thrkey_setData(l_thrkey* self, const void* data);

l_spec_specif(void*)
l_thrkey_getData(l_thrkey* self);

l_spec_specif(void)
l_mutex_init(l_mutex* self);

l_spec_specif(void)
l_mutex_free(l_mutex* self);

l_spec_specif(void)
l_mutex_lock(l_mutex* self);

l_spec_specif(void)
l_mutex_unlock(l_mutex* self);

l_spec_specif(int)
l_mutex_tryLock(l_mutex* self);

l_spec_specif(void)
l_rwlock_init(l_rwlock* self);

l_spec_specif(void)
l_rwlock_free(l_rwlock* self);

l_spec_specif(void)
l_rwlock_read(l_rwlock* self);

l_spec_specif(void)
l_rwlock_write(l_rwlock* self);

l_spec_specif(int)
l_rwlock_tryRead(l_rwlock* self);

l_spec_specif(int)
l_rwlock_tryWrite(l_rwlock* self);

l_spec_specif(void)
l_rwlock_unlock(l_rwlock* self);

l_spec_specif(void)
l_condv_init(l_condv* self);

l_spec_specif(void)
l_condv_free(l_condv* self);

l_spec_specif(void)
l_condv_wait(l_condv* self, l_mutex* mutex);

l_spec_specif(int)
l_condv_timedWait(l_condv* self, l_mutex* mutex, l_long ns);

l_spec_specif(void)
l_condv_signal(l_condv* self);

l_spec_specif(void)
l_condv_broadcast(l_condv* self);

l_spec_specif(l_thrid)
l_raw_thread_self();

l_spec_specif(int)
l_raw_thread_create(l_thrid* thrid, void* (*start)(void*), void* para);

l_spec_specif(int)
l_raw_thread_join(l_thrid* thrid);

l_spec_specif(void)
l_raw_thread_cancel(l_thrid* thrid);

l_spec_specif(void)
l_raw_thread_exit();

l_spec_specif(void)
l_thread_sleep(l_long us);


#endif /* lucy_core_thread_h */
