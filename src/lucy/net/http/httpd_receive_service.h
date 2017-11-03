#ifndef l_net_httpd_receive_service_h
#define l_net_httpd_receive_service_h
#include "core/net/http/httpd_listen_service.h"

#define L_HTTP_BACKLOG (32)
#define L_HTTP_RDREQ_STAGE (1)
#define L_HTTP_WRRES_STAGE (2)
#define L_HTTP_WRITE_STATUS (3)
#define L_HTTP_WRITE_HEADER (4)
#define L_HTTP_WRITE_BODY (5)

typedef struct {
  l_handle sock;
  l_int rx_limit;
  l_string* buf;
  l_int lstart; /* line start */
  l_int lnewline; /* newline pos */
  l_int lend; /* line end */
  l_int mstart; /* match start */
} l_http_read_common;

typedef struct {
  l_int start;
  l_int end;
} l_startend;

typedef struct {
  const l_byte* arg;
  const l_byte* mid;
  const l_byte* end;
} l_urlarg;

typedef struct {
  l_service head;
  l_httpd_listen_service* ss;
  l_ushort rmtFamily;
  l_ushort rmtPort;
  l_byte rmtAddr[16];
  l_http_read_common comm;
  l_string rxbuf;
  int stage;
  l_byte method; /* method */
  l_byte httpver; /* http version */
  l_byte isfolder;
  l_byte urldepth;
  l_int suffix, suffixend; /* suffix pointer to '.' */
  l_int arg; /* arg pointer to '?' */
  l_int url, uend; /* request url */
  l_int body, bodylen; /* body */
  l_umedit commasks;
  l_umedit reqmasks;
  l_startend comhead[32];
  l_startend reqhead[32];
  l_string txbuf;
  l_byte* txcur;
} l_httpd_receive_service;

#endif /* l_net_httpd_receive_service_h */

