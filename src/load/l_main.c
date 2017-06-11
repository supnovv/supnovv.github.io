#include "service.h"

struct ccionfslot {
  struct ccsmplnode head;
};

struct ccionfnode {
  struct ccionfnode* bucket_next_dont_use_;
  struct ccionfnode* qnext; /* union with size/type/flag in ccmsghead */
};

struct ccpqueue {
  struct ccionfnode head;
  struct ccionfnode* tail;
};

struct ccionfpool {
  umedit_int nslot; /* prime number size not near 2^n */
  umedit_int nfreed, nbucket, qsize;
  struct ccpqueue queue; /* chain all ccionfmsg fifo */
  struct ccsmplnode freelist;
  struct ccionfslot slot[1];
};

struct ccionfmgr {
  struct ccionfpool* pool;
};

struct ccionfevt;
struct ccionfmsg;

nauty_bool ccionfevt_isempty(struct ccionfevt* self);
void ccionfevt_setempty(struct ccionfevt* self);
struct ccionfmsg* ccionfmsg_new(struct ccionfevt* event);
struct ccionfmsg* ccionfmsg_set(struct ccionfmsg* self, struct ccionfevt* event);

void ccpqueue_init(struct ccpqueue* self) {
  self->head.qnext = self->tail = &self->head;
}

void ccpqueue_push(struct ccpqueue* self, struct ccionfnode* newnode) {
  newnode->qnext = self->tail->qnext;
  self->tail->qnext = newnode;
  self->tail = newnode;
}

nauty_bool ccpqueue_isempty(struct ccpqueue* self) {
  return (self->head.qnext == &self->head);
}

struct ccionfnode* ccpqueue_pop(struct ccpqueue* self) {
  struct ccionfnode* node = 0;
  if (ccpqueue_isempty(self)) {
    return 0;
  }
  node = self->head.qnext;
  self->head.qnext = node->qnext;
  if (self->tail == node) {
    self->tail = &self->head;
  }
  return node;
}

#define CCSOCK_ACCEPT 0x01
#define CCSOCK_CONNET 0x02

#define CCMSGTYPE_SOCKMSG 1
#define CCMSGFLAG_FREEMEM 0x0001
#define CCMSGFLAG_NEWCONN 0x0100
#define CCMSGFLAG_CONNEST 0x0200

struct ccsockmsg* ccnewsockmsg(int sockfd, umedit_int events, void* ud) {
  struct ccsockmsg* sm = (struct ccsockmsg*)ccrawalloc(sizeof(struct ccsockmsg));
  sm->head.size = sizeof(struct ccsockmsg);
  sm->head.type = CCMSGTYPE_SOCKMSG;
  sm->head.flag = CCMSGFLAG_FREEMEM;
  sm->head.data = ud;
  sm->sockfd = sockfd;
  sm->events = events;
  return sm;
}

void ccsocknewconn(struct ccepoll* self, int connfd, struct ccwork* work) {
  struct ccsockmsg* sm = ccnewsockmsg(connfd, 0, ud);
  sm->head.flag |= CCMSGFLAG_NEWCONN;
}

void ccepolldispatch(struct ccepoll* self) {
  int i = 0, n = self->n;
  struct epoll_event* ready = self->ready;
  struct ccevent* e = 0;
  for (; i < n; ++i) {
    e = (struct ccevent*)ready[i].data.ptr;
    e->revents = (umedit_int)ready[i].events;
    if ((e->revents | CCEPOLLIN) && (e->waitop & CCSOCK_ACCEPT)) {
      ccsockaccept(self, e);
    }
  }
}

struct ccionf {
  int epfd;
  int n, maxlen;
  struct epoll_event ready[CCIONF_MAX_WAIT_EVENTS];
};

struct ccionfevt {
  int fd;
  umedit_int events;
  void* ud;
};

struct ccionfmsg {
  struct ccmsghead head;
  struct ccionfevt event;
};

nauty_bool ccionfevtvoid(struct ccionfevt* self) {
  return (self->fd == -1);
}

struct ccionfmsg* ccionfmsgnew(struct ccionfevt* event) {
  struct ccionfmsg* self = (struct ccionfmsg*)ccrawalloc(sizeof(struct ccionfmsg));
  return ccionfmsgset(self, event);
}

struct ccionfmsg* ccionfmsgset(struct ccionfmsg* self, struct ccionfevt* event) {
  self->event = *event;
  return self;
}


umedit_int llionfpool_hash(struct ccionftbl* self, umedit_int fd) {
  return fd % self->size; /* size should be prime number not near 2^n */
}

umedit_int llionfpool_size(uoctect_int bits) {
  /* cchashprime(bits) return a prime number < (1 << bits) */
  return cchashprime(bits);
}

void llionfpool_addtofreelist(struct ccionftbl* self, struct ccsmplnode* node) {
  ccsmplnode_insertafter(&self->freelist, node);
  self->nfreed += 1;
}

struct ccionfmsg* llionfpool_getfromfreelist(struct ccionftbl* self, struct ccionfevt* event) {
  struct ccionfmsg* p = 0;
  if (((p = struct ccionfmsg*)ccsmplnode_removenext(&self->freelist)) == 0) {
    return 0;
  }
  if (self->nfreed > 0) {
    self->nfreed -= 1;
  } else {
    ccloge("ccionfpool freelist size invalid");
  }
  return llionfmsg_set(p, event);
}

struct ccionfpool* ccionfpool_new(uoctect_int sizenumbits) {
  struct ccionfpool* pool = 0;
  sright_int size = llionfpool_size(sizenumbits);
  pool = (struct ccionfpool*)ccrawalloc(sizeof(struct ccionfpool) + size * sizeof(struct ccionfslot));
  pool->nslot = size;
  while (size > 0) {
    ccsmplnode_init(&(pool->slot[--size].head));
  }
  ccsmplnode_init(&pool->freelist);
  ccpqueue_init(&pool->queue);
  pool->nfreed = pool->nbucket = pool->qsize = 0;
  return pool;
}

struct ccsmplnode* ccionfpool_addevent(struct ccionfpool* self, struct ccionfevt* newevent) {
  struct ccionfmsg* msg = 0;
  struct ccionfslot* slot = 0;
  struct ccsmplnode* head = 0;
  struct ccsmplnode* cur = 0;
  if (llionfevt_isempty(newevent)) {
    ccloge("addEvent invalid event");
    return 0;
  }
  slot = self->slot + ccionfpool_hash(self, (umedit_int)newevent->fd);
  for (head = &slot->head, cur = head; cur->next != head; cur = cur->next) {
    msg = (struct ccionfmsg*)cur->next;
    /* if current event is new, than it will lead all empty nodes freed */
    if (llionfevt_isempty(&msg->event)) {
      llionfpool_addtofreelist(self, ccsmplnode_removenext(cur));
      if (self->nbucket > 0) {
        self->nbucket -= 1;
      } else {
        ccloge("ccionfpool bucket size invalid");
      }
      continue;
    }
    if (msg->event.fd == newevent->fd) {
      msg->event.events |= newevent->events;
      return 0;
    }
  }
  if ((msg = llionfpool_getfromfreelist(self, newevent)) == 0) {
    msg = ccepollmsg_new(newevent);
  }
  ccsmplnode_insertafter(&slot->node, (struct ccsmplnode*)msg);
  self->nbucket += 1;
  ccpqueue_push(&self->queue, msg);
  self->qsize += 1;
  return msg;
}

int ccnodemain(struct ccstate* s) {
  struct ccglobal G;
  ccinitglobal(&G, "ccnode.conf");
  struct ccstate s[G.workers+1];
  int i = 0;
  ccinitstatepool(&G, s+1, G.workers);
  for (; i < G.workers + 1; ++i) {

  }
  ccinitstate(&s[0], G, pthread_self());
  G.iofd = llepollcreate();
  if (G.iofd == -1) return -1;
}

int main(int argc, char** argv) {
  return startmainthreadcv(ccmaster_start, argc, argv);
}

