#ifndef l_thread_lib_h
#define l_thread_lib_h

struct ccmutex {
  CCPLAT_IMPL_SIZE(CC_MUTEX_BYTES);
};

struct ccrwlock {
  CCPLAT_IMPL_SIZE(CC_RWLOCK_BYTES);
};

struct cccondv {
  CCPLAT_IMPL_SIZE(CC_CONDV_BYTES);
};

struct ccthrid {
  CCPLAT_IMPL_SIZE(CC_THRID_BYTES);
};

struct ccthrkey {
  CCPLAT_IMPL_SIZE(CC_THKEY_BYTES);
};

/**
 * thread-specific data key
 */

CORE_API nauty_bool ccthrkey_init(struct ccthrkey* self);
CORE_API void ccthrkey_free(struct ccthrkey* self);
CORE_API nauty_bool ccthrkey_setdata(struct ccthrkey* self, const void* data);
CORE_API void* ccthrkey_getdata(struct ccthrkey* self);

/**
 * mutex
 */

CORE_API nauty_bool ccmutex_init(struct ccmutex* self);
CORE_API void ccmutex_free(struct ccmutex* self);
CORE_API nauty_bool ccmutex_lock(struct ccmutex* self);
CORE_API void ccmutex_unlock(struct ccmutex* self);
CORE_API nauty_bool ccmutex_trylock(struct ccmutex* self);

/**
 * read/write lock
 */

CORE_API nauty_bool ccrwlock_init(struct ccrwlock* self);
CORE_API void ccrwlock_free(struct ccrwlock* self);
CORE_API nauty_bool ccrwlock_read(struct ccrwlock* self);
CORE_API nauty_bool ccrwlock_write(struct ccrwlock* self);
CORE_API nauty_bool ccrwlock_tryread(struct ccrwlock* self);
CORE_API nauty_bool ccrwlock_trywrite(struct ccrwlock* self);
CORE_API void ccrwlock_unlock(struct ccrwlock* self);

/**
 * condition variable
 */

CORE_API nauty_bool cccondv_init(struct cccondv* self);
CORE_API void cccondv_free(struct cccondv* self);
CORE_API nauty_bool cccondv_wait(struct cccondv* self, struct ccmutex* mutex);
CORE_API nauty_bool cccondv_timedwait(struct cccondv* self, struct ccmutex* mutex, sright_int ns);
CORE_API void cccondv_signal(struct cccondv* self);
CORE_API void cccondv_broadcast(struct cccondv* self);

/**
 * thread
 */

CORE_API struct ccthrid ccplat_selfthread();
CORE_API nauty_bool ccplat_createthread(struct ccthrid* thrid, void* (*start)(void*), void* para);
CORE_API void ccplat_threadsleep(uright_int us);
CORE_API void ccplat_threadexit();
CORE_API int ccplat_threadjoin(struct ccthrid* thrid);

#endif /* l_thread_lib_h */

