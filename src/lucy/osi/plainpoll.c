struct l_epoll {
  l_array fdset;
  l_hashtbl fdud;
};

#define L_EPOLL_INIT_FDBITS 6  /* 2 ^ 6 = 64 */
#define L_EPOLL_INIT_FDSIZE (1 << L_EPOLL_INIT_FDBITS)

void l_epollinit(l_epoll* self) {
  l_zeron(self, sizeof(l_epoll));
}

int l_epollcreate(l_epoll* self) {
  l_arrayinit(&self->fdset, sizeof(struct pollfd), L_EPOLL_INIT_FDSIZE);
  l_hashtblinit(&self->fdud, L_EPOLL_INIT_FDBITS);
  return true;
}

void l_epollclose(l_epoll* self) {
  l_arrayfree(&self->fdset);
  l_hashtblfree(&self->fdud);
}

#define L_EPOLLIN POLLIN
#define L_EPOLLPRI POLLPRI
#define L_EPOLLOUT POLLOUT
#define L_EPOLLERR (POLLERR | POLLNVAL)
#define L_EPOLLHUP POLLHUP

int l_epolladd(l_epoll* self, int fd, char flag) {
  struct pollfd event;
  event.fd = fd;
  event.revents = 0;
  event.events = POLLIN | POLLPRI;
  if (flag == 'a') {
    event.events |= POLLOUT;
  } else if (flag == 'w') {
    event.events = POLLOUT;
  } else if (flag != 'r') {
    event.events = 0;
    l_loge_s("epolladd invalid falg");
    return false;
  }
  return l_arrayadd(&self->fdset, &event);
}

int l_epollmod(l_epoll* self, int fd, char flag) {
  struct pollfd event = {0};
  event.fd = fd;
  struct pollfd* p = l_arrayfind(&self->fdset, fd, l_epollequalfunc);
  if (p == 0) return;
  p->revents = 0;
  p->events = POLLIN | POLLPRI;
  if (flag == 'a') {
    event.events |= POLLOUT;
  } else if (flag == 'w') {
    event.events = POLLOUT;
  } else if (flag != 'r') {
    event.events = 0;
    l_loge_s("epollmod invalid flag");
  }
}

void l_epolldel(l_epoll* self, int fd) {
  struct pollfd event = {0};
  event.fd = fd;
  l_arraydel(&self->fdset, &event, l_epollequalfunc);
}

void l_epollwait(l_epoll* self) {
  l_epollwaitms(self, INFTIM);
}

void l_epolltrywait(l_epoll* self) {
  l_epollwaitms(self, 0);
}

void l_epollwaitms(l_epoll* self, int ms) {
  poll(self->fdset.buffer, self->fdset.nelem, ms);
}

