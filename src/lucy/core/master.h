#ifndef lucy_core_master_h
#define lucy_core_master_h

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
  l_file logfile;
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

l_inline lua_State* l_get_luastate() {
  return l_thread_self()->L;
}

l_extern l_thread* l_thread_master();
l_extern int l_thread_start(l_thread* self, int (*start)());
l_extern int l_thread_join(l_thread* self);
l_extern void l_thread_flush_log(l_thread* self);
l_extern void l_thread_exit();
l_extern void l_process_exit();


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
  void* ptr;
  l_int n;
} l_message_ptrn;

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
  int (*entry)(l_service*, l_message*); /* service entry function */
  void (*destroy)(l_service*); /* destroy function for child structure's fields if necessary */
  /* coroutine */
  lua_State* co;
  int (*func)(l_service*);
  int (*kfunc)(l_service*);
  int coref;
} l_service;

l_extern l_service* l_create_service(l_int size, int (*entry)(l_service*, l_message*), void (*destroy)(l_service*));
l_extern l_service* l_create_service_in_same_thread(l_int size, int (*entry)(l_service*, l_message*), void (*destroy)(l_service*));
l_extern int l_free_unstarted_service(l_service* srvc);
l_extern void l_start_service(l_service* srvc);
l_extern void l_start_listener_service(l_service* srvc, l_handle sock);
l_extern void l_start_initiator_service(l_service* srvc, l_handle sock);
l_extern void l_start_receiver_service(l_service* srvc, l_handle sock);
l_extern void l_close_service(l_service* srvc);
l_extern void l_close_event(l_service* srvc);


l_extern int startmainthread(int (*start)());
l_extern int startmainthreadcv(int (*start)(), int argc, char** argv);
l_extern void l_master_exit();


#endif /* lucy_core_master_h */

