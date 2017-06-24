#ifndef l_service_lib_h
#define l_service_lib_h
#include "thatcore.h"
#include "l_message.h"

#define L_SERVICE_MASTERID (0)

/*
1. 队列不能直接赋值
2. frsv只需要master拥有即可
3. 需要标记service能不能释放，如果srvc->ioev没释放就不释放srvc
4. state resume 返回错误时释放该co，并重新分配一个全新co

假如现在masks不为0且一个消息已经发送给了线程，然后有来了一个事件：
1. 如果master在检查masks前，线程还没有处理这个事件消息 - masks != 0, master update masks
2. 如果master在检查masks前，线程已经处理了这个事件消息 - masks == 0, master update masks, and send a new message
线程关闭事件文件描述符时:
1. 如果还有对应的事件消息(最多1个)在线程等待队列中，masks必定非0
   此时也可以将service->ioev设为0，因为后面线程处理消息时看到service->ioev已经不存在了，直接忽略这个消息即可
2. masks已经为0，但是master可能正在等待线程释放锁再发送一个事件消息
   此时将service->ioev设为0，master获得锁后发现service->ioev已经不存在了，会直接忽略这个事件
线程在关闭服务时：
1. 如果还有事件消息在线程队列中待处理（masks不为0）
   此时可以先将service->ioev设为0，因为后面线程处理消息时发送看到service->ioev不存在了，直接忽略这个消息即可（但前提是这时候service还没有释放）
2. 如果买有事件消息在等待（masks为0），但master可能正在等待线程释放锁在发送一个事件消息
   此时将service->ioev设为0是安全的,并发送关闭文件描述符消息，和第1种情况一样（不能发送关闭服务消息是因为要等到线程中所有这个服务的消息都处理完毕了）
怎么在释放服务前保证线程中的事件消息处理完毕了
1. 线程将service->ioev设为0之后，master不可能再发送事件消息给线程了
2. 因此线程在处理事件消息时如果发现这个service->ioev已经为0，则忽略这个消息，此时该服务的事件消息都处理完毕了
怎么判断什么时候服务的普通消息都处理完毕了呢？
1. 服务上设置一个共享标记stopmsg，线程在关闭服务时，先将这个标记设为1，此后任何其他服务都不能给这个服务发送消息了
2. 然后线程给master发送一个 prepare_stop_service 消息，然后master收到这个消息后，stopmsg必定为1
3. master只在stopmsg为1时给线程返回一个准备好关闭服务的消息，然后将stopmsg设为2，注意这个消息一定是发给给服务的最后一个消息
4. 线程处理这个消息使，就可以安全的发送 stop_service 消息给master去关闭服务了，因为给服务的最后一个消息都处理完了
*/

typedef struct l_service {
  L_COMMON_BUFHEAD
  l_ioevent* ioev; /* guard by svmtx */
  l_ioevent event; /* guard by svmtx */
  l_byte stop_rx_msg; /* service is closing, guard by svmtx */
  l_byte mflgs; /* only accessed by master */
  /* thread own use */
  l_byte wflgs; /* only accessed by a worker */
  l_umedit svid; /* only set once when init, so can freely access it */
  l_thread* belong; /* only set once when init, so can freely access it */
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

#endif /* l_service_lib_h */

