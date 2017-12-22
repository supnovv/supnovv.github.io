#define L_LIBRARY_IMPL
#include "core/net/http/httpd_listen_service.h"

#define L_HTTP_DEFAULT_PORT 80
#define L_HTTP_DEFAULT_TX_INIT_SIZE 1024
#define L_HTTP_DEFAULT_RX_INIT_SIZE 128
#define L_HTTP_DEFAULT_RX_LIMIT 1024*8

static int
l_httpd_listen_service_proc(l_service* self, l_message* msg)
{
  l_connind_message* connind = (l_connind_message*)msg;
  l_httpd_listen_service* ss = (l_httpd_listen_service*)self;

  switch (msg->msgid) {
  case L_MSGID_SERVICE_START:
    return 0;
  case L_MSGID_SERVICE_CLOSE: /* TODO: free service resources here */
    return 0;
  default:
    if (msg->type != L_MESSAGE_CONNIND || l_filedesc_isEmpty(l_msg_getfd(msg))) {
      l_logw_2("httpd_listen_service_proc msg %d fd %d", ld(msg->msgid), ld(l_msg_getfd(msg)));
      return L_STATUS_EINVAL;
    }
    break;
  }

  return l_httpd_receive_conn(ss, connind);
}

static int
l_httpd_listen_service_getip(void* self, l_strt str)
{
  l_httpd_listen_service* ss = (l_httpd_listen_service*) self;
  if (l_strt_isEmpty(&str)) return false;
  ss->ip = l_string_createFrom(str);
  return true;
}

static int
l_httpd_listen_service_getmod(void* self, l_strt str)
{
  l_httpd_listen_service* ss = (l_httpd_listen_service*) self;
  if (l_strt_isEmpty(&str)) return false;
  ss->lua_module = l_string_createFrom(str);
  return true;
}

L_EXTERN int
l_httpd_start(const void* http_conf_name, int (*client_request_handler)(l_service*))
{
  l_sockaddr sa;
  l_httpd_listen_service* ss = 0;
  lua_State* L = l_get_luastate();

  ss = L_SERVICE_CREATE(l_httpd_listen_service);

  if (!http_conf_name) {
    http_conf_name = "http_default";
  }

  if (!l_luaconf_strv(L, l_httpd_listen_service_getip, ss, 2, http_conf_name, "ip")) {
    ss->ip = l_string_createFrom(l_strt_literal("0.0.0.0"));
  }

  if ((ss->port = l_luaconf_intv(L, 2, http_conf_name, "port")) == 0) {
    ss->port = L_HTTP_DEFAULT_PORT;
  }

  if ((ss->backlog = l_luaconf_intv(L, 2, http_conf_name, "backlog")) < L_HTTP_BACKLOG) {
    ss->backlog = L_HTTP_BACKLOG;
  }

  if ((ss->tx_init_size = l_luaconf_intv(L, 2, http_conf_name, "tx_init_size")) < L_HTTP_DEFAULT_TX_INIT_SIZE) {
    ss->tx_init_size = L_HTTP_DEFAULT_TX_INIT_SIZE;
  }

  if ((ss->rx_init_size = l_luaconf_intv(L, 2, http_conf_name, "rx_init_size")) < L_HTTP_DEFAULT_RX_INIT_SIZE) {
    ss->rx_init_size = L_HTTP_DEFAULT_RX_INIT_SIZE;
  }

  if ((ss->rx_limit = l_luaconf_intv(L, 2, http_conf_name, "rx_limit")) < L_HTTP_DEFAULT_RX_LIMIT) {
    ss->rx_limit = L_HTTP_DEFAULT_RX_LIMIT;
  }

  ss->client_request_handler = 0;
  if (!l_luaconf_strv(L, l_httpd_listen_service_getmod, ss, 2, http_conf_name, "lua_module")) {
    ss->lua_module = l_string_empty();
    ss->client_request_handler = client_request_handler;
  }

  if (!l_sockaddr_init(&sa, l_string_strt(&ss->ip), ss->port)) {
    l_service_close(&ss->head);
    return L_STATUS_ERROR;
  }

  ss->sock = l_socket_listen(&sa, ss->backlog);
  if (l_socket_isEmpty(ss->sock)) {
    l_service_close(&ss->head);
    return L_STATUS_ERROR;
  }

  l_logm_8("start http server: %s ip %s port %d backlog %d tx_init_size %d rx_init_size %d rx_limit %d module %s",
    ls(http_conf_name), ls(l_string_start(&ss->ip)), ld(ss->port), ld(ss->backlog), ld(ss->tx_init_size),
    ld(ss->rx_init_size), ld(ss->rx_limit), l_string_isEmpty(&ss->lua_module) ? ls("n/a") : ls(l_string_start(&ss->lua_module)));

  l_service_setListen(&ss->head, ss->sock);
  l_service_start(&ss->head);
  return 0;
}

