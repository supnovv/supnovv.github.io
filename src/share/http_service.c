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


#if 0
"<scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<frag>"
/hammers;sale=0;color=red?item=1&size=m
/hammers;sale=false/index.html;graphics=true
/check.cgi?item=12731&color=blue&size=large
/tools.html#drills

URL编码机制，为了使用不安全字符，使用%和两个两个十六进制字符表示，例如空格可使用%20表示，根据浏览器的不同组合成的字符可能使用不同的编码
保留字符有 % / . .. # ? ; : $ + @ & =
受限字符有 {} | \ ^ ~ [ ] ' < > "　以及不可打印字符 0x00~0x1F >=0x7F，而且不能使用空格，通常使用+来替代空格
0~9A~Za~z_-

---
<method> <request-url> HTTP/<major>.<minor>
<headers> header: value<crlf>
<crlf>
<entity-body>
---
HTTP/<major>.<minor> <status-code> <status-text>
<headers> header: value<crlf>
<crlf>
<entity-body>
---
#endif

