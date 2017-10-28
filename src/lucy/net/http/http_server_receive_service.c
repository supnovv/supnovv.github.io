#define L_LIBRARY_IMPL
#include "core/net/http/http_read_socket.h"
#include "core/net/http/http_server_receive_service.h"

static int l_http_server_receive_service_proc(l_service* srvc, l_message* msg);
static void l_http_server_discard_data(l_http_server_receive_service* ssrx);
static int l_http_server_read_request(l_service* srvc);
static int l_http_server_read_headers(l_service* srvc);

L_PRIVAT int
l_http_server_receive_conn(l_http_server_listen_service* ss, l_connind_message* msg)
{
  l_http_server_receive_server* ssrx = 0;
  ssrx = L_SERVICE_CREATE(l_http_server_receive_service);
  ssrx->ss = ss;
  ssrx->rmtFamily = l_connind_getFamily(msg);
  ssrx->rmtPort = l_connind_getPort(msg);
  l_copy_n(msg->addr, 16, ssrx->rmtAddr);
  ssrx->comm.sock = l_connind_getSock(msg);
  ssrx->comm.rx_limit = ss->rx_limit;
  ssrx->comm.buf = &ssrx->rxbuf;
  ssrx->rxbuf = l_string_createEx(ss->rx_init_size, ss->thread);
  ssrx->txbuf = l_string_createEx(ss->tx_init_size, ss->thread);
  l_string_setLimit(&ssrx->rxbuf, ss->rx_limit);
  l_service_setResume(&ssrx->head, l_http_server_read_request);
  l_service_setEvent(&ssrx->head, ssrx->comm.sock, L_SOCKET_RDWR);
  l_service_start(&ssrx->head);
  return 0;
}

static int
l_http_server_receive_service_proc(l_service* srvc, l_message* msg)
{
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_umedit masks = 0;
  int n = 0;

  switch (msg->msgid) {
  case L_MSGID_SERVICE_START:
    return 0;
  case L_MSGID_SERVICE_CLOSE: /* TODO: free service resources here */
    return 0;
  default:
    if (msg->msgid != L_MSGID_SOCK_EVENT_IND) {
      l_loge_1("http_server_receive_service_proc msg %d", ld(msg->msgid));
      return L_STATUS_EINVAL;
    }
    break;
  }

  masks = msg->data;
  if (masks & (L_IOEVENT_ERR | L_IOEVENT_HUP | L_IOEVENT_RDH)) {
    l_service_close(srvc);
    return 0;
  }

  /* TODO: there may be multiple events stored in masks */

  if (ssrx->stage < L_HTTP_WRRES_STAGE) {
    int n = 0;
    if (!(masks & L_IOEVENT_READ)) {
      return 0;
    }
    if ((n = l_service_resume(srvc)) != 0) {
      if (n < 0) l_close_service(srvc);
      return 0;
    }
    ssrx->stage = L_HTTP_WRRES_STAGE;
    l_service_setResume(srvc, ssrx->server->client_request_handler);
  } else {
    if (masks & L_IOEVENT_READ) {
      /* read event received at writing response stage, read it out and discard */
      l_loge_s("read and discard data at WRRES_STAGE");
      l_http_server_discard_data(ssrx);
    }
    if (!(masks & L_IOEVENT_WRITE)) {
      return 0;
    }
  }

  if ((n = l_service_resume(srvc)) != 0) {
    if (n < 0) l_close_service(srvc);
    return 0;
  }

  /* current request handled finished, prepare for next request or disconnect */
  ssrx->stage = L_HTTP_RDREQ_STAGE;
  if (ssrx->httpver <= L_HTTP_VER_10N) {
    l_close_service(srvc);
  }
  return 0;
}

static void
l_http_server_discard_data(l_http_server_receive_service* ssrx)
{
  l_int n = 0;
  l_int status = 0;
  #define L_HTTP_DISCARD_BUFFER_SIZE 1024
  l_byte buffer[L_HTTP_DISCARD_BUFFER_SIZE+1];
ContinueRead:
  n = l_socket_read(ssrx->comm.sock, buffer, L_HTTP_DISCARD_BUFFER_SIZE, &status);
  if (status < 0) return; /* status >=0 success, <0 L_STATUS_ERROR */
  if (n == L_HTTP_DISCARD_BUFFER_SIZE) {
    goto ContinueRead;
  }
  return;
}

static int
l_http_server_read_request(l_service* srvc)
{
  /* <method> <request-url> HTTP/<major>.<minor><crlf> #CarriageReturn(CR) 13 '\r' #LineFeed(LF) 10 '\n' */
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_http_read_common* comm = &ssrx->comm;
  l_string* rxbuf = &ssrx->rxbuf;
  const l_rune* buff_start = 0;
  const l_rune* line_end = 0;
  const l_rune* match_end = 0;
  l_rune* first_non_space_pos = 0;
  l_int strid = 0;
  int status = 0;

  if ((status = l_http_read_line(comm)) < 0) {
    return status;
  }

  if (status == L_STATUS_WAITMORE) {
    return l_service_yield(srvc, l_http_server_read_request);
  }

  /* a line is read, parse <method> first */
  buff_start = l_string_start(rxbuf);
  line_end = buff_start + comm->lnewline;
  match_end = l_string_skip_space_and_match(l_http_method_map(), l_strt_e(buff_start + comm->lstart, line_end), &strid, 0);
  if (!match_end) {
    l_logw_1("unsupported method %s", lp(buff_start + comm->lstart));
    return L_STATUS_EMATCH;
  }
  ssrx->method = strid;

  /* parse <request-url> */
  match_end = l_string_skip_space_and_match_until(l_string_blank_map(), l_strt_e(match_end, line_end), &first_non_space_pos);
  if (!match_end) return L_STATUS_EMATCH;
  ssrx->url = first_non_space_pos - buff_start;
  ssrx->uend = match_end - buff_start;

  /* parse <http-ver> */
  match_end = l_string_skip_space_and_match(l_http_version_map(), l_strt_e(match_end, line_end), &strid, 0);
  if (!match_end) {
    ssrx->httpver = L_HTTP_VER_0NN;
    return 0; /* v0.9请求报文只包括<method>和<request-url>，没有版本、头部、和主体；而响应报文只包含主体 */
  }
  ssrx->httpver = strid;

  /* start to parse headers */
  return l_http_server_read_headers(srvc);
}

static int l_http_server_read_headers(l_service* srvc) {
  /* <name>: <value><crlf>
     ...
     <name>: <value><crlf>
     <crlf>
     <entity-body>  */
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_http_read_common* comm = &ssrx->comm;
  l_string* rxbuf = &ssrx->rxbuf;
  const l_rune* buff_start = 0;
  const l_rune* line_end = 0;
  const l_rune* match_end = 0;
  l_strt headval, cur_line;
  l_int headid = 0;
  int status = 0;

  for (; ;) {
    if ((status = l_http_read_line(comm)) < 0) {
      return status;
    }

    if (status == L_STATUS_WAITMORE) {
      return l_service_yield(srvc, l_http_server_read_headers);
    }

    buff_start = l_string_start(rxbuf);
    line_end = buff_start + comm->lnewline;
    cur_line = l_strt_e(buff_start + comm->lstart, line_end);

    match_end = l_string_match_repeat(l_string_space_map(), cur_line);
    if (match_end == line_end) {
      break; /* current line is empty line, headers ended here */
    }

    if ((headid = l_http_match_header(l_http_common_map(), cur_line, &headval)) >= 0) {
      ssrx->commasks |= (1 << headid);
      ssrx->comhead[headid].start = headval.start - buff_start;
      ssrx->comhead[headid].end = headval.end - buff_start;
      if (headid == L_HTTP_H_CONTENT_LENGTH) {
        ssrx->bodylen = l_string_parse_dec(headval);
      }
      continue;
    }

    if ((headid = l_http_match_header(l_http_request_map(), cur_line, &headval) >= 0)) {
      ssrx->reqmasks |= (1 << headid);
      ssrx->reqhead[headid].start = headval.start - buff_start;
      ssrx->reqhead[headid].end = headval.end - buff_start;
      continue;
    }

    l_logw_1("unknown or invalid header %strt", lstrt(&cur_line));
  }

  if (ssrx->method < L_HTTP_M_POST) {
    return 0; /* these method don't have body */
  }

  ssrx->body = comm->lend;

  if (ssrx->comhead[L_HTTP_H_TRANSFER_ENCODING].start) {
    return l_http_read_chunked_body(srvc);
  }

  return l_http_read_body(srvc);
}

