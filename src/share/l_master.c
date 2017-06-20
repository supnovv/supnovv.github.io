
static l_ionfmgr llg_ionf_mgr;
static l_thread* llg_master_thread;
static l_squeue llg_msg_rxq;
static l_mutex llg_msg_lock;

void l_globalmsgq_create() {
  l_squeue_init(&llg_msg_rxq);
  l_mutex_init(&llg_msg_lock);
}

void l_globalmsgq_destroy() {
  l_message* msg = 0;
  while ((msg = (l_message*)l_squeue_pop(&llg_msg_rxq))) {
    l_raw_free(msg);
  }
  l_mutex_free(&llg_msg_lock);
}

l_squeue l_global_messages() {
  l_squeue q;
  l_squeue* mq = &llg_msg_rxq;
  l_mutex* mtx = &llg_msg_lock;

  l_mutex_lock(mtx);
  q = *mq;
  l_squeue_init(mq);
  l_mutex_lock(mtx);

  return q;
}

void llsendmessages(l_thread* thread) {
  l_mutex* mtx = &llg_msg_lock;

  l_mutex_lock(mtx);
  l_squeue_push_queue(&llg_msg_rxq, &thread->txmq);
  l_mutex_unlock(mtx);

  /* wakeup master to handle the message */
  l_ionfmgr_wakeup(ionf);
}

static l_service* llfindservice(l_umedit svid) {

}

static void ll_master_send_connrsp_message(l_umedit dstid, l_event* ev) {
  l_thread* master = l_master_thread();
  l_message* msg = l_create_message(master, L_MESSAGE_CONNRSP, sizeof(l_ioevent_message);
  l_connrsp_message* p = (l_connrsp_message*)msg;
  p->sock = ev->fd;
  p->masks = ev->masks;
  l_send_message(master, dstid, msg);
}

static void ll_accept_connection(void* ud, l_sockconn* conn) {
  l_service* sv = (l_service*)ud;
  l_thread* master = l_master_thread();
  l_message* msg = l_create_message(master, L_MESSAGE_CONNIND, sizeof(l_connind_message));
  l_connind_message* p = (l_connind_message*)msg;
  p->sock = conn->sock;
  p->remote = conn->remote;
  l_send_message(master, sv->svid, msg);
}

static void ll_master_dispatch_ioevent(l_ioevent* ev) {
  l_service* srvc = 0;
  l_ioevent* svio = 0;
  l_thread* thread = 0;
  l_mutex* emtx = 0;
  l_umedit type = 0;
  int unchained = false;

  srvc = ll_master_find_service(ev->udata);
  if (!srvc) {
    l_loge_s("service not found");
    return;
  }

  thread = srvc->belong;
  if (!thread) {
    l_loge_s("service not run");
    return;
  }

  svio = &srvc->io;
  if (svio->fd != ev->fd) {
    l_loge_s("io event mismatch");
    return;
  }

  emtx = thread->elock;
  l_mutex_lock(emtx);
  if (svio->falgs & L_EVENT_FLAG_LISTEN) {
      type = L_MESSAGE_CONNIND;
  } else if (svio->flags & L_EVENT_FLAG_CONNECT) {
      svio->flags &= (~L_EVENT_FLAG_CONNECT);
      type = L_MESSAGE_CONNRSP;
  } else {
    l_thread* master = l_master_thread();
    type = L_MESSAGE_IOEVENT;
    svio->masks |= ev->masks;
    if (!svio->chained) {
      svio->chained = srvc;
      unchained = true;
    }
  }
  l_mutex_unlock(emtx);

  if (unchained) {
    l_squeue_push(&master->rxio, &svio->node);
  }

  if (type == L_MESSAGE_CONNIND) {
    l_socket_accept(sv->event->fd, ll_accept_connection, sv);
  } else if (type == L_MESSAGE_CONNRSP) {
    ll_master_send_connrsp_message(srvc->dstid, ev);
  }
}

static void ll_master_handle_message(l_squeue* frmq) {
  l_thread* master = l_master_thread();
  l_message* msg = 0;
  l_squeue msgq;

  l_squeue_init(&msgq);
  l_thread_lock(master);
  l_squeue_push_queue(&msgq, &master->rxmq);
  l_thread_unlock(master);

  while ((msg = (l_message*)l_squeue_pop(msgq))) {
    switch (msg->type) {
    case L_MESSAGE_RUNSERVICE:
      sv = (l_service*)msg->data.ptr;
      if (sv->event) {
        l_ionfmgr_add(ll_get_ionfmgr(), sv->event);
      }
      ll_add_service_to_table(sv);
      break;
    case L_MESSAGE_ADDEVENT:
      if (msg->data.ptr) {
        l_ioevent* event = (l_ioevent*)msg->data.ptr;
        l_ionfmgr_add(ll_get_ionfmgr(), event);
      }
      break;
    case L_MESSAGE_DELEVENT:
      if (msg->data.ptr) {
        l_ioevent* event = (l_ioevent*)msg->data.ptr;
        l_ionfmgr_del(ll_get_ionfmgr(), event);
        /* need free the event after it is deleted */
        ll_free_event(event);
      }
      break;
    case L_MESSAGE_DELSERVICE:
      sv = (l_service*)msg->data.ptr;
      /* service's received msgs (if any) */
      l_squeue_pushqueue(&freeq, &sv->rxmq);
      /* detach and free event if any */
      if (sv->event) {
        l_ionfmgr_del(ll_get_ionfmgr(), sv->event);
        ll_free_event(sv->event);
        sv->event = 0;
      }
      /* remove service out from hash table and insert to free queue */
      ll_service_is_finished(sv);
      /* reset the service */
      sv->belong = 0;
      break;
    default:
      l_loge("unknow message id");
      break;
    }
    l_squeue_push(&freeq, &msg->node);
  }
}

static void ll_check_service_completed(l_service* srvc) {
  if (sv->belong) {
        nauty_bool done = false;

        ll_lock_thread(sv->belong);
        done = (sv->outflag & L_SERVICE_COMPLETED) != 0;
        ll_unlock_thread(sv->belong);

        if (done) {
          /* free current msg and service's received msgs (if any) */
          l_squeue_push(&freeq, &msg->node);
          l_squeue_pushqueue(&freeq, &sv->rxmq);

          /* when service is done, the service is already removed out from thread's service queue.
          here delete service from the hash table and insert into service's free queue */
          ll_service_is_finished(sv);

          /* reset the service */
          sv->belong = 0;
          continue;
        }
      } else {
        /* attach a thread to handle the service */
        sv->belong = ll_get_thread_for_new_service();
        newattach = true;
      }
}

void l_master_start() {
  l_squeue rxmq, frmq;
  l_message* msg = 0;
  l_service* srvc = 0;
  l_thread* thread = 0;
  l_thread* master = l_master_thread();
  l_ioevent* ioev = 0;
  l_mutex* emtx = 0;

  for (; ;) {
    l_ionfmgr_wait(ll_get_ionfmgr(), l_master_dispatch_event);

    /* handle master's messages */
    l_squeue_init(&freeq);
    ll_master_handle_message(&freeq);

    /* dispatch service's messages */
    rxmq = ll_get_current_msgs();
    l_squeue_push_queue(&rxmq, &master->txmq);
    while ((msg = (l_message*)l_squeue_pop(&rxmq))) {

      /* get message's dest service */
      srvc = ll_find_service(msg->dstid);
      if (srvc == 0) {
        l_loge_1("service not found %d", ld(msg->dstid));
        l_squeue_push(&freeq, &msg->node);
        continue;
      }

      if ((ll_check_service_completed(service)) {
        continue;
      }

      thread = srvc->belong;
      msg->extra = srvc;
      l_thread_lock(thread);
      l_squeue_push(&thread->rxmq, &msg->node);
      if (!thread->msgwait) {
        thread->msgwait = 1;
        l_condv_signal(&thread->condv);
      }
      l_thread_unlock(thread);
    }

    l_free_messages(master, &frmq);

    /* dispatch io event */
    while ((ioev = (l_ioevent*)l_squeue_pop(&master->rxio))) {
      srvc = ioev->chained;
      thread = srvc->belong;

      if ((ll_check_service_completed(service)) {
        ioev->chained = 0;
        ioev->masks = 0;
        continue;
      }

      emtx = thread->elock;
      l_mutex_lock(emtx);
      l_squeue_push(&thread->rxio, &ioev->node);
      l_mutex_unlock(emtx);

      l_thread_lock(thread);
      if (!thread->iowait) {
        thread->iowait = 1;
        l_condv_signal(&thread->condv);
      }
      l_thread_unlock(thread);
    }
  }
}

static void ll_deliver_service_msg(l_service* srvc, l_message* msg) {
  l_thread* thread = srvc->belong;

  srvc->entry(srvc, msg);
  if (!(srvc->flag & L_SERVICE_COMPLETED)) return;

  if (srvc->co) {
    ll_free_coroutine(thread, service->co);
    service->co = 0;
  }

  ll_lock_thread(thread);
  service->outflag |= L_SERVICE_COMPLETED;
  ll_unlock_thread(thread);

  /* currently the finished service is removed out from thread's
  service queue, but the service still pointer to this thread.
  send SVDONE message to master to reset and free this service. */
  ll_service_send_message_sp(service, L_SERVICE_MASTERID, L_MESSAGE_DELSERVICE, service);
}

static void ll_thread_flush_messages(l_thread* self) {
  l_message* msg = 0;
  l_thread* master = l_master_thread();
  l_mutex* mtx = &llg_msg_lock;
  l_squeue* txmq = &self->txmq;
  l_squeue* txms = &self->txms;
  int sent = false;

  if (!l_squeue_is_empty(txms)) {
    l_thread* master = l_master_thread();
    l_thread_lock(master);
    l_squeue_push_queue(&master->rxmq, txms);
    l_thread_unlock(master);
    sent = true;
  }

  if (!l_squeue_is_empty(txmq)) {
    l_mutex_lock(mtx);
    l_squeue_push_queue(&llg_msg_rxq, txmq);
    l_mutex_unlock(mtx);
    sent = true;
  }

  if (sent) {
    ll_wakeup_master();
  }
}

/* SERVICE要么保存在hashtable中，要么保存在free队列中，
一个SERVICE只分配给一个线程处理，当某个SERVICE的消息到来
时, 从消息可以获取SERVICE号，由SERVICE号从hashtable中
取得service对象，这个service对象会保存到消息extra中，然
后将消息插入线程的消息队列中等待线程处理。一个SERVICE只接收
一个文件描述符的IO事件通知。*/

void l_worker_start() {
  l_squeue msgq, frmq, rxio;
  l_service* service = 0;
  l_message* msg = 0;
  l_thread* thread = l_thread_self();
  l_mutex* emtx = 0;
  l_ioevent* ioev = 0;
  l_ioevent ev;
  l_ioevent_message iomsg;

  l_squeue_init(&msgq);
  l_squeue_init(&rxio);

  for (; ;) {
    l_thread_lock(thread);
    while (l_squeue_is_empty(&thread->rxmq) && l_squeue_is_empty(&thread->rxio)) {
      thread->msgwait = thread->iowait = 0;
      l_condv_wait(&thread->condv, &thread->mutex);
    }
    l_squeue_push_queue(&msgq, &thread->rxmq);
    l_squeue_push_queue(&rxio, &thread->rxio);
    l_thread_unlock(thread);

    l_squeue_init(&svdq);
    l_squeue_init(&frmq);

    while ((msg = (l_message*)l_squeue_pop(&msgq))) {
      service = (l_service*)msg->extra;
      if (service->flag & L_SERVICE_COMPLETED) {
        l_squeue_push(&frmq, &msg->node);
        continue;
      }

      ll_deliver_service_msg(service, msg);

      /* free handled message */
      l_squeue_push(&freemq, &msg->node);
    }

    emtx = thread->elock;

    while ((ioev = (l_iovent*)l_squeue_pop(&rxio)) {
      /* event's content except 'node' field is guarded by elock */
      l_mutex_lock(emtx);
      ev = *ioev;
      ioev->masks = 0;
      ioev->chained = 0;
      l_mutex_unlock(emtx);

      service = ev.chained;

      iomsg.extra = service;
      iomsg.dstid = service.dstid;
      iomsg.type = L_MESSAGE_IOEVENT;
      iomsg.sock = ev.fd;
      iomsg.masks = ev.masks;
      service->entry(service, &iomsg);

      ll_deliver_service_msg(service, msg);
    }

    l_free_messages(thread, &frmq);

    ll_thread_flush_messages(thread);
  }
}

static void llparsecommandline(int argc, char** argv) {
  (void)argc;
  (void)argv;
}

int startmainthreadcv(int (*start)(), int argc, char** argv) {
  int n = 0;
  l_global G;
  l_thread master;

  /* init global first */
  l_global_init(&G);

  /* init master thread */
  l_thread_init(&master);
  master.id = l_plat_selfthread();

  /* attach master thread */
  l_global_setmaster(&master);

  /* call start func */
  llparsecommandline(argc, argv);
  n = start();

  /* clean */
  l_thread_free(&master);
  l_global_free(&G);
  return n;
}

int l_master_start(int (*start)(), int argc, char** argv) {
  return startmainthreadcv(start, argc, argv);
}

