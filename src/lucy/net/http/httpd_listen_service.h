#ifndef l_net_httpd_listen_service_h
#define l_net_httpd_listen_service_h
#include "core/service.h"

typedef struct {
  l_service head;
  l_string ip;
  l_ushort port;
  l_handle sock;
  int backlog;
  l_int tx_init_size;
  l_int rx_init_size;
  l_int rx_limit;
  int (*client_request_handler)(l_service*);
  l_int slots;
  l_hashtable* strpool;
} l_httpd_listen_service;

L_PRIVAT int l_httpd_receive_conn(l_httpd_listen_service* ss, l_connind_message* msg);

#endif /* l_net_httpd_listen_service_h */

