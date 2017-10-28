#ifndef l_net_http_read_socket_h
#define l_net_http_read_socket_h

L_EXTERN int l_http_read_length(l_http_read_common* comm, l_int len);
L_EXTERN int l_http_read_line(l_http_read_common* comm);
L_EXTERN int l_http_read_chunked_body(l_service* srvc);
L_EXTERN int l_http_read_body(l_service* srvc);

#endif /* l_net_http_read_socket_h */

