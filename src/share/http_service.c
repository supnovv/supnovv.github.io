#include "socket.h"

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

int　ll_http_receive_connection(struct ccstate* s) {
  struct httplisten* self = (struct httplisten*)robot_get_specific(s);
  struct ccmessage* msg = robot_get_message(s);
  if (msg->type == MESSAGE_ID_CONNIND) {
    struct httpconnect* conn;
    struct ccrobot* robot;
    conn = (struct httpconnect*)ccrawalloc_init(sizeof(struct httpconnect));
    conn->sock = msg->data.fd;
    robot = robot_create(0, self->connind, ROBOT_YIELDABLE);
    robot_set_allocated_specific(robot, conn);
    robot_attach_event(robot, conn->sock, IOEVENT_RDWR, 0);
  } else {
    ccloge("http invalid message");
  }
  return ROBOT_CONTINUE;
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
  robot = robot_create(0, ll_http_receive_connection);
  robot->udata = self;
  robot_attach_event(robot, sock, IOEVENT_READ, IOEVENT_FLAG_LISTEN);
  return robot;
}


