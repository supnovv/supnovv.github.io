#ifndef lucy_core_h
#define lucy_core_h
#include "autoconf.h"
#include "core/prefix.h"

#undef l_inline
#define l_inline static

#undef l_extern
#if defined(L_BUILD_SHARED)
  #if defined(__GNUC__)
  #define l_extern extern
  #else
  #if defined(L_CORELIB_IMPL) || defined(L_WINDOWS_IMPL)
  #define l_extern __declspec(dllexport)
  #else
  #define l_extern __declspec(dllimport)
  #endif
  #endif
#else
  #define l_extern extern
#endif

#undef l_specif
#define l_specif l_extern

// This directive is currently not supported on Mac OS X (it will give
// a compiler error), since compile-time TLS is not supported in the Mac OS X
// executable format. Also, some older versions of MinGW (before GCC 4.x) do
// not support this directive.
// fixme: Check for a PROPER value of __STDC_VERSION__ to know if we have C11
//#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
// #if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
//  #define _Thread_local __thread
// #else
//  #define _Thread_local __declspec(thread)
// #endif
//#endif

#undef l_thread_local
#undef l_extern_thread_local
#undef l_thread_local_supported
#if defined(L_CLER_GCC)
  #define l_thread_local(a) __thread a
  #define l_extern_thread_local(a) extern __thread a
  #define l_thread_local_supported
#elif defined(L_CLER_MSC)
  #define l_thread_local(a) __declspec(thread) a
  #define l_extern_thread_local(a) extern __declspec(thread) a
  #define l_thread_local_supported
#else
  #define l_thread_local(a)
#endif

#define l_cast(type, a) ((type)(a))
#define l_rstr(s) ((l_rune*)(s))

#define l_max_rdwr_size (0x7fff0000) /* 2147418112 */
#define l_max_ubyte     l_cast(l_byte, 0xff) /* 255 */
#define l_max_ushort    l_cast(l_ushort, 0xffff) /* 65535 */
#define l_max_umedit    l_cast(l_umedit, 0xffffffff) /* 4294967295 */
#define l_max_ulong     l_cast(l_ulong, 0xffffffffffffffff) /* 18446744073709551615 */
#define l_max_sbyte     l_cast(l_sbyte, 0x7f) /* 127 */
#define l_max_short     l_cast(l_short, 0x7fff) /* 32767 */
#define l_max_medit     l_cast(l_medit, 0x7fffffff) /* 2147483647 */
#define l_max_long      l_cast(l_long, 0x7fffffffffffffff) /* 9223372036854775807 */
#define l_min_sbyte     l_cast(l_sbyte, -127-1) /* 128 0x80 */
#define l_min_short     l_cast(l_short, -32767-1) /* 32768 0x8000 */
#define l_min_medit     l_cast(l_medit, -2147483647-1) /* 2147483648 0x80000000 */
#define l_min_long      l_cast(l_long, -9223372036854775807-1) /* 9223372036854775808 0x8000000000000000 */

#define L_STATUS_CONTREAD (3)
#define L_STATUS_WAITMORE (2)
#define L_STATUS_YIELD (1)
#define L_STATUS_OK (0)
#define L_STATUS_LUAERR (-1)
#define L_STATUS_ERROR (-2)
#define L_STATUS_EREAD (-3)
#define L_STATUS_EWRITE (-4)
#define L_STATUS_ELIMIT (-5)
#define L_STATUS_EMATCH (-6)
#define L_STATUS_EINVAL (-7)

#ifdef L_BUILD_DEBUG
#define L_DEBUG_HERE(...) { __VA_ARGS__ }
#define l_debug_assert(e) l_assert(e)
#else
#define L_DEBUG_HERE(...) { ((void)0); }
#define l_debug_assert(e) ((void)0)
#endif

#define l_zero_e(start, end) l_zero_l(start, l_str(end) - l_str(start))
l_extern void l_zero_l(void* start, l_int len);

#define l_copy_e(from, end, to) return l_copy_l(from, l_str(end) - l_str(from), (to))
l_extern l_byte* l_copy_l(const void* from, l_int len, void* to);

l_extern void* l_raw_malloc(l_int size);
l_extern void* l_raw_calloc(l_int size);
l_extern void* l_raw_realloc(void* buffer, l_int oldsize, l_int newsize);
l_extern void l_raw_free(void* buffer);

typedef struct l_linknode {
  struct l_linknode* next;
  struct l_linknode* prev;
} l_linknode;

typedef struct l_smplnode {
  struct l_smplnode* next;
} l_smplnode;

l_extern void l_linknode_init(l_linknode* node);
l_extern void l_linknode_insert_after(l_linknode* node, l_linknode* newnode);
l_extern int l_linknode_is_empty(l_linknode* node);
l_extern l_linknode* l_linknode_remove(l_linknode* node);

l_extern void l_smplnode_init(l_smplnode* node);
l_extern void l_smplnode_insert_after(l_smplnode* node, l_smplnode* newnode);
l_extern int l_smplnode_is_empty(l_smplnode* node);
l_extern l_smplnode* l_smplnode_remove_next(l_smplnode* node);

typedef struct l_squeue {
  l_smplnode head;
  l_smplnode* tail;
} l_squeue;

typedef struct l_dqueue {
  l_linknode head;
} l_dqueue;

l_extern void l_squeue_init(l_squeue* self);
l_extern void l_squeue_push(l_squeue* self, l_smplnode* newnode);
l_extern void l_squeue_push_queue(l_squeue* self, l_squeue* queue);
l_extern int l_squeue_is_empty(l_squeue* self);
l_extern l_smplnode* l_squeue_pop(l_squeue* self);

l_extern void l_dqueue_init(l_dqueue* self);
l_extern void l_dqueue_push(l_dqueue* self, l_linknode* newnode);
l_extern void l_dqueue_push_queue(l_dqueue* self, l_dqueue* queue);
l_extern int l_dqueue_is_empty(l_dqueue* self);
l_extern l_linknode* l_dqueue_pop(l_dqueue* self);

typedef struct l_priorq {
  l_linknode node;
  /* elem with less number has higher priority, i.e., 0 is the highest */
  int (*less)(void* elem_is_less_than, void* this_one);
} l_priorq;

l_extern void l_priorq_init(l_priorq* self, int (*less)(void*, void*));
l_extern void l_priorq_push(l_priorq* self, l_linknode* elem);
l_extern void l_priorq_remove(l_priorq* self, l_linknode* elem);
l_extern int l_priorq_is_empty(l_priorq* self);
l_extern l_linknode* l_priorq_pop(l_priorq* self);

/* min.max. heap - add and remove quick, search slow */
typedef struct {
  l_umedit size;
  l_umedit capacity;
  l_int* a; /* array of elements with pointer size */
  int (*less)(void* this_elem_is_less, void* than_this_one);
} l_mmheap;

/* min. heap - pass less function to less, max. heap - pass greater function to less */
l_extern void l_mmheap_init(l_mmheap* self, int (*less)(void*, void*), int initsize);
l_extern void l_mmheap_free(l_mmheap* self);
l_extern void l_mmheap_add(l_mmheap* self, void* elem);
l_extern void* l_mmheap_del(l_mmheap* self, l_umedit i);

#define L_COMMON_BUFHEAD \
  l_smplnode node; \
  l_int bsize;

#include "core/string.h"

typedef struct {
  void* stream;
} l_filestream;

l_extern l_filestream l_open_read(const void* name);
l_extern l_filestream l_open_write(const void* name);
l_extern l_filestream l_open_append(const void* name);
l_extern l_filestream l_open_read_write(const void* name);
l_extern l_filestream l_open_read_unbuffered(const void* name);
l_extern l_filestream l_open_write_unbuffered(const void* name);
l_extern l_filestream l_open_append_unbuffered(const void* name);
l_extern l_int l_write_strt_to_file(l_filestream* self, l_strt s);
l_extern l_int l_write_file(l_filestream* self, const void* s, l_int len);
l_extern l_int l_read_file(l_filestream* self, void* out, l_int len);

l_extern int l_remove_file(const void* name);
l_extern int l_rename_file(const void* from, const void* to);
l_extern void l_redirect_stdout(const void* name);
l_extern void l_redirect_stderr(const void* name);
l_extern void l_reditect_stdin(const void* name);
l_extern void l_close_file(l_filestream* self);
l_extern void l_flush_file(l_filestream* self);
l_extern void l_rewind_file(l_filestream* self);
l_extern void l_seek_from_begin(l_filestream* self, long offset);
l_extern void l_seek_from_curpos(l_filestream* self, long offset);
l_extern void l_clear_file_error(l_filestream* self);

/**
 * The 64-bit signed integer's biggest value is 9223372036854775807.
 * For seconds/milliseconds/microseconds/nanoseconds, it can
 * represent more than 291672107014/291672107/291672/291 years.
 * The 32-bit signed integer's biggest value is 2147483647.
 * For seconds/milliseconds/microseconds/nanoseconds, it can
 * represent more than 67-year/24-day/35-min/2-sec.
 */

#define l_nsecs_per_second (1000000000L)

typedef struct {
  l_long sec;
  l_umedit nsec;
} l_time;

typedef struct {
  l_umedit year; /* 38-bit, can represent 274877906943 years */
  l_umedit nsec; /* nanoseconds that less than 1 sec */
  l_ushort yday; /* 1 ~ 366 */
  l_byte high;   /* extra bits for year */
  l_byte wdmon;  /* 1~12, high 4-bit is wday (0~6, 0 is sunday) */
  l_byte day;    /* 1~31 */
  l_byte hour;   /* 0~23 */
  l_byte min;    /* 0~59 */
  l_byte sec;    /* 0~61, 60 and 61 are the leap seconds */
} l_date;

l_specif l_time l_system_time();
l_specif l_time l_monotonic_time();
l_specif l_date l_system_date();
l_specif l_date l_date_from_secs(l_long utcsecs);
l_specif l_date l_date_from_time(l_time utc);

typedef struct {
  l_long fsize;
  l_long ctm; /* creation, utc */
  l_long atm; /* last access, utc */
  l_long mtm; /* last modify, utc */
  l_long gid;
  l_long uid;
  l_long mode;
  l_byte isfile;
  l_byte isdir;
  l_byte islink;
} l_fileattr;

typedef struct {
  void* stream;
} l_dirstream;

l_specif l_long l_file_size(const void* name);
l_specif l_fileattr l_file_attr(const void* name);
l_specif l_dirstream l_open_dir(const void* name);
l_specif void l_close_dir(l_dirstream* d);
l_specif const l_rune* l_read_dir(l_dirstream* d);
l_specif int l_print_current_dir(void* stream, int (*write)(void* stream, const void* str));

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

l_specif void l_thrkey_init(l_thrkey* self);
l_specif void l_thrkey_free(l_thrkey* self);
l_specif void l_thrkey_set_data(l_thrkey* self, const void* data);
l_specif void* l_thrkey_get_data(l_thrkey* self);

l_specif void l_mutex_init(l_mutex* self);
l_specif void l_mutex_free(l_mutex* self);
l_specif void l_mutex_lock(l_mutex* self);
l_specif void l_mutex_unlock(l_mutex* self);
l_specif int l_mutex_trylock(l_mutex* self);

l_specif void l_rwlock_init(l_rwlock* self);
l_specif void l_rwlock_free(l_rwlock* self);
l_specif void l_rwlock_read(l_rwlock* self);
l_specif void l_rwlock_write(l_rwlock* self);
l_specif int l_rwlock_tryread(l_rwlock* self);
l_specif int l_rwlock_trywrite(l_rwlock* self);
l_specif void l_rwlock_unlock(l_rwlock* self);

l_specif void l_condv_init(l_condv* self);
l_specif void l_condv_free(l_condv* self);
l_specif void l_condv_wait(l_condv* self, l_mutex* mutex);
l_specif int l_condv_timedwait(l_condv* self, l_mutex* mutex, l_long ns);
l_specif void l_condv_signal(l_condv* self);
l_specif void l_condv_broadcast(l_condv* self);

l_specif l_thrid l_raw_thread_self();
l_specif int l_raw_thread_create(l_thrid* thrid, void* (*start)(void*), void* para);
l_specif int l_raw_thread_join(l_thrid* thrid);
l_specif void l_raw_thread_cancel(l_thrid* thrid);
l_specif void l_raw_thread_exit();
l_specif void l_thread_sleep(l_long us);

typedef struct lua_State lua_State;
typedef struct l_service l_service;

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
  l_filestream logfile;
  void* frbq;
  l_thrid id;
  int (*start)();
  void* block;
} l_thread;

l_extern l_thrkey L_thread_key;
l_extern_thread_local(l_thread* L_thread_ptr);

l_inline l_thread* l_thread_self() {
#if defined(l_thread_local_supported)
  return L_thread_ptr;
#else
  return (l_thread*)l_thrkey_get_data(&L_thread_key);
#endif
}

l_extern l_thread* l_thread_master();
l_extern int l_thread_start(l_thread* self, int (*start)());
l_extern int l_thread_join(l_thread* self);
l_extern void l_thread_flush_log(l_thread* self);
l_extern void l_thread_exit();
l_extern void l_process_exit();

#include "core/socket.h"

#define L_IOEVENT_READ  0x01
#define L_IOEVENT_WRITE 0x02
#define L_IOEVENT_RDWR  0x03
#define L_IOEVENT_PRI   0x04
#define L_IOEVENT_RDH   0x08
#define L_IOEVENT_HUP   0x10
#define L_IOEVENT_ERR   0x20

#define L_IOEVENT_FLAG_ADDED   0x01
#define L_IOEVENT_FLAG_LISTEN  0x02
#define L_IOEVENT_FLAG_CONNECT 0x04

typedef struct l_ioevent {
  l_handle fd;
  l_umedit udata;
  l_ushort masks;
  l_ushort flags;
} l_ioevent;

typedef struct {
  L_PLAT_IMPL_SIZE(L_IONFMGR_SIZE);
} l_ionfmgr;

l_extern int l_ionfmgr_init(l_ionfmgr* self);
l_extern void l_ionfmgr_free(l_ionfmgr* self);
l_extern int l_ionfmgr_add(l_ionfmgr* self, l_ioevent* event);
l_extern int l_ionfmgr_mod(l_ionfmgr* self, l_ioevent* event);
l_extern int l_ionfmgr_del(l_ionfmgr* self, l_handle fd);
l_extern int l_ionfmgr_wait(l_ionfmgr* self, void (*cb)(l_ioevent*));
l_extern int l_ionfmgr_trywait(l_ionfmgr* self, void (*cb)(l_ioevent*));
l_extern int l_ionfmgr_timedwait(l_ionfmgr* self, int ms, void (*cb)(l_ioevent*));
l_extern int l_ionfmgr_wakeup(l_ionfmgr* self);


typedef struct l_state {
  L_COMMON_BUFHEAD
  l_thread* thread;
  lua_State* co;
  int coref;
  int (*func)(struct l_state*);
  int (*kfunc)(struct l_state*);
  l_service* srvc;
} l_state;

l_inline l_thread* l_state_belong(l_state* s) {
  return s->thread;
}

l_extern l_int lucy_intconf(lua_State* L, int n, ...);
l_extern int lucy_strconf(lua_State* L, int (*func)(void* stream, const void* str), void* stream, int n, ...);

l_extern lua_State* l_new_luastate();
l_extern void l_close_luastate(lua_State* L);
l_extern int l_state_init(l_state* co, l_thread* thread, l_service* srvc, int (*func)(l_state*));
l_extern void l_state_free(l_state* co);
l_extern int l_state_resume(l_state* co);
l_extern int l_state_yield(l_state* co, int (*kfunc)(l_state*));
l_extern int l_state_yield_with_code(l_state* co, int (*kfunc)(l_state*), int code);


#define L_MESSAGE_START_SERVICE 0x01
#define L_MESSAGE_START_SRVCRSP 0x02
#define L_MESSAGE_CLOSE_SERVICE 0x03
#define L_MESSAGE_CLOSE_SRVCRSP 0x04
#define L_MESSAGE_CLOSE_EVENTFD 0x05
#define L_MESSAGE_BOOTSTRAP 0x06
#define L_MESSAGE_MASTER_EXIT 0x07
#define L_MESSAGE_WORKER_EXIT 0x08
#define L_MESSAGE_WORKER_EXITRSP 0x09
#define L_MESSAGE_IOEVENT 0x7d
#define L_MESSAGE_CONNRSP 0x7e
#define L_MESSAGE_CONNIND 0x7f
#define L_MIN_USER_MSGID  0x80

typedef struct l_message {
  L_COMMON_BUFHEAD
  l_service* srvc;
  l_umedit dstid;
  l_umedit type;
} l_message;

typedef struct {
  l_message head;
  l_handle fd;
  l_umedit masks;
  l_sockaddr remote;
} l_connind_message;

typedef struct {
  l_message head;
  l_handle fd;
  l_umedit masks;
} l_ioevent_message;

typedef struct {
  l_message head;
  l_umedit svid;
  l_handle fd;
} l_service_message;

typedef struct {
  l_message head;
  int (*bootstrap)();
} l_bootstrap_message;

typedef struct {
  l_message head;
  l_handle fd;
} l_message_fd;

typedef struct {
  l_message head;
  void* ptr;
} l_message_ptr;

typedef struct {
  l_message head;
  l_umedit u32;
} l_message_u32;

typedef struct {
  l_message head;
  l_medit s32;
} l_message_s32;

typedef struct {
  l_message head;
  l_ulong u64;
} l_message_u64;

typedef struct {
  l_message head;
  l_long s64;
} l_message_s64;

l_extern l_message* l_create_message(l_thread* thread, l_umedit type, l_int size);
l_extern void l_free_message(l_thread* thread, l_message* msg);
l_extern void l_free_message_queue(l_thread* thread, l_squeue* mq);
l_extern void l_send_message(l_thread* thread, l_umedit destid, l_message* msg);
l_extern void l_send_message_tp(l_thread* thread, l_umedit destid, l_umedit type);
l_extern void l_send_message_fd(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd);
l_extern void l_send_message_ptr(l_thread* thread, l_umedit destid, l_umedit type, void* ptr);
l_extern void l_send_message_u32(l_thread* thread, l_umedit destid, l_umedit type, l_umedit u32);
l_extern void l_send_message_u64(l_thread* thread, l_umedit destid, l_umedit type, l_ulong u64);
l_extern void l_send_message_s32(l_thread* thread, l_umedit destid, l_umedit type, l_medit s32);
l_extern void l_send_message_s64(l_thread* thread, l_umedit destid, l_umedit type, l_long s64);
l_extern void l_send_ioevent_message(l_thread* thread, l_umedit destid, l_umedit type, l_handle fd, l_umedit masks);
l_extern void l_send_service_message(l_thread* thread, l_umedit destid, l_umedit type, l_umedit svid, l_handle fd);
l_extern void l_send_bootstrap_message(l_thread* thread, int (*bootstrap)());


#define L_SERVICE_MASTER_ID (0) /* worker's default svid is its index (16-bit) */
#define L_SERVICE_BOOTSTRAP (0xffff+1)

typedef struct l_service {
  L_COMMON_BUFHEAD
  l_ioevent* ioev; /* guard by svmtx */
  l_ioevent event; /* guard by svmtx */
  l_byte stop_rx_msg; /* service is closing, guard by svmtx */
  l_byte mflgs; /* only accessed by master */
  /* thread own use */
  l_byte wflgs; /* only accessed by a worker */
  l_umedit svid; /* only set once when init, so can freely access it */
  l_thread* thread; /* only set once when init, so can freely access it */
  l_state* co; /* only accessed by a worker */
  int (*entry)(l_service*, l_message*); /* only accessed by a worker */
} l_service;

l_extern l_service* l_create_service(l_thread* thread, l_int size, int (*entry)(l_service*, l_message*), int samethread);
l_extern void l_start_service(l_service* srvc);
l_extern void l_start_listener_service(l_service* srvc, l_handle sock);
l_extern void l_start_initiator_service(l_service* srvc, l_handle sock);
l_extern void l_start_receiver_service(l_service* srvc, l_handle sock);
l_extern void l_close_service(l_service* srvc);
l_extern void l_close_event(l_service* srvc);
l_extern int l_service_resume(l_service* self, int (*func)(l_state*));
l_extern int l_service_yield(l_service* self, int (*kfunc)(l_state*));


l_extern int startmainthread(int (*start)());
l_extern int startmainthreadcv(int (*start)(), int argc, char** argv);
l_extern void l_master_exit();


l_extern void l_core_test();
l_extern void l_string_test();
l_extern void l_luac_test();
l_specif void l_plat_test();
l_specif void l_plat_ionf_test();

#endif /* lucy_core_h */
