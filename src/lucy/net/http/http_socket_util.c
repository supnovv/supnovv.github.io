#define L_LIBRARY_IMPL
#include "core/net/http/http_socket_util.h"

#define L_HTTP_ESTIMATED_LINE_LENGTH (128)

L_EXTERN int
l_http_read_length(l_http_read_common* comm, l_int len)
{
  l_string* rxbuf = comm->buf;
  l_int count = 0, n = 0, status = 0;
  l_rune* buff_start = 0;

  comm->lstart = comm->lend; /* move forward to last end position */

ContinueCheckLength:
  if (l_string_size(rxbuf) - comm->lstart >= len) {
    comm->lend = comm->lstart + len;
    return 0;
  }

  if (count > 0 && n < count) return L_STATUS_WAITMORE; /* last time socket read indicates there is no more data */

  if (!l_string_ensure_capacity(rxbuf, comm->lstart + len + 1)) {
    return L_STATUS_ELIMIT;
  }

  buff_start = l_string_start(rxbuf);
  count = l_string_remain(rxbuf) - 1;
  n = l_socket_read(comm->sock, buff_start + l_string_size(rxbuf), count, &status);
  if (status < 0) return L_STATUS_EREAD;

  rxbuf->b->size += n; /* may read more data beyond newline */
  *(buff_start + rxbuf->b->size) = 0;
  goto ContinueCheckLength;
}

L_EXTERN int
l_http_read_line(l_http_read_common* comm)
{
  l_string* rxbuf = comm->buf;
  l_rune* buff_start = 0;
  const l_rune* match_end = 0;
  l_rune* last_match_start = 0;
  l_int count = 0, n = 0, status = 0;

  comm->lstart = comm->lend; /* move forward to last line end */

ContinueMatch:

  buff_start = l_string_start(rxbuf);
  match_end = l_string_match_until(l_string_newline_map(), l_strt_sft(buff_start, comm->mstart, l_string_size(rxbuf)), &last_match_start);
  if (match_end) { /* newline matched */
    comm->lnewline = last_match_start - buff_start;
    comm->lend = match_end - buff_start;
    comm->mstart = comm->lend;
    return 0;
  }

  comm->mstart = last_match_start - buff_start; /* next time match start here */
  if (count > 0 && n < count) return L_STATUS_WAITMORE; /* last time socket read indicates there is no more data */

  if (!l_string_ensure_remain(rxbuf, L_HTTP_ESTIMATED_LINE_LENGTH)) {
    return L_STATUS_ELIMIT;
  }

  buff_start = l_string_start(rxbuf);
  count = l_string_remain(rxbuf) - 1;
  n = l_socket_read(comm->sock, buff_start + l_string_size(rxbuf), count, &status);
  if (status < 0) return L_STATUS_EREAD;

  rxbuf->b->size += n; /* may read more data beyond newline */
  *(buff_start + rxbuf->b->size) = 0;
  goto ContinueMatch;
}

l_int l_http_match_header(l_stringmap* name, l_strt s, l_strt* value) {
  const l_rune* match_end = 0;
  l_int headid = 0;

  if ((match_end = l_string_match_ex(name, s, &headid, 0)) < s.start) {
    return L_STATUS_EMATCH;
  }

  s.start = match_end;
  if (!(match_end = l_string_skip_space_and_match_sub(l_literal_strt(":"), s))) {
    return L_STATUS_EMATCH;
  }

  s.start = match_end;
  if (!(value->start = l_string_trim_head(s))) return L_STATUS_EINVAL;
  value->end = s.end;
  return headid;
}

L_EXTERN int
l_http_read_chunked_body(l_service* srvc)
{
  /* Transfer-Encoding: chunked<CRLF>
     Trailer: Content-MD5<CRLF>
     <CRLF>
     f<CRLF>
     123456789abcdef<CRLF>
     2<CRLF>
     ab<CRLF>
     0<CRLF>
     Content-MD5:gjqei54p26tjisgj3p4utjgrj53<CRLF> */
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  (void)srvc;
  (void)ssrx;
  return 0;
}

L_EXTERN int
l_http_read_body(l_service* srvc)
{
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_http_read_common* comm = &ssrx->comm;
  int status = 0;

  if (ssrx->bodylen <= 0) return 0;

  if ((status = l_http_read_length(comm, ssrx->bodylen)) < 0) {
    return status;
  }

  if (status == L_STATUS_WAITMORE) {
    return l_service_yield(srvc, l_http_read_body);
  }

  return 0;
}

