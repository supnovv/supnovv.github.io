#include "socket.h"

/* 三种robot类型
1. 不接收消息，也不监听事件，只挂在当前线程下，与同线程下的多个robot协同工作（通过调用robot_resume和robot_yield）
2. 接收消息，不监听事件，可以与同线程robot协同工作
3. 接收消息，也监听事件，也可以与通线程robot协同工作
*/

#define HTTP_LISTEN_BACKLOG (32)

struct httplisten {
  struct ccstring ip;
  ushort_int port;
  void　(*connind)(struct ccstate*);
  int backlog;
};

struct httpconnect {
  handle_int sock;
  struct ccstring localip;
  struct ccstring remoteip;
  ushort_int localport;
  ushort_int remoteport;
};

nauty_bool ll_http_filter_connection(struct ccstate* state, handle_int sock, struct httpconnect* conn) {

}

void　ll_http_receive_connection(struct ccstate* s) {
  struct httplisten* self = (struct httplisten*)robot_get_specific(s);
  struct ccmessage* msg = robot_get_message(s);
  struct httpconnect conn;
  struct httpconnect* specific = 0;
  struct ccrobot* robot = 0;
  ccassert(msg->type == MESSAGE_ID_CONNIND);
  if (!ll_http_filter_connection(s, msg->data->fd, &conn)) {
    return;
  }
  robot = robot_create_new(self->connind, ROBOT_YIELDABLE);
  specific = (struct httpconnect*)robot_set_allocated_specific(robot, sizeof(struct httpconnect));
  *specific = conn;
  robot_set_event(robot, conn.sock, IOEVENT_RDWR, 0);
  robot_start_run(robot);
}

struct ccrobot* http_listen(struct httplisten* self) {
  struct ccsockaddr sa;
  handle_int sock;
  struct ccrobot* robot;
  if (!ccsockaddr_init(&sa, ccstring_getfrom(&self->ip), self->port)) {
    return 0;
  }
  if (self->backlog <= 0) {
    self->backlog = HTTP_LISTEN_BACKLOG;
  }
  sock = ccsocket_listen(&sa, self->backlog);
  if (!ccsocket_isopen(sock)) {
    return 0;
  }
  robot = robot_create_new(ll_http_receive_connection, 0);
  robot->udata = self;
  robot_set_event(robot, sock, IOEVENT_READ, IOEVENT_FLAG_LISTEN);
  robot_start_run(robot);
  return robot;
}

void user_read_request_header(struct ccstate* state) {
  robot_yield(state, user_read_request_data);
}


