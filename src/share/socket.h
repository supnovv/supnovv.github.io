#ifndef l_socket_lib_h
#define l_socket_lib_h
#include "thatcore.h"

#define L_SOCKET_BACKLOG  (32)
#define L_SOCKET_IPSTRLEN (48)

l_extern int l_sockaddr_init(l_sockaddr* self, l_strt ip, l_ushort port);
l_extern l_ushort l_sockaddr_port(l_sockaddr* self);
l_extern int l_sockaddr_ipstring(l_sockaddr* self, l_string* out);

l_extern l_handle l_socket_listen(const l_sockaddr* addr, int backlog);
l_extern void l_socket_accept(l_handle sock, void (*cb)(void*, l_sockconn*), void* ud);
l_extern void l_socket_close(l_handle sock);
l_extern void l_socket_shutdown(l_handle sock, l_rune r_w_a);
l_extern void l_socketconn_init(l_sockconn* self, l_strt ip, l_ushort port);

l_extern int l_socket_connect(l_sockconn* conn);
l_extern int l_socket_isopen(l_handle sock);
l_extern l_sockaddr l_socket_localaddr(l_handle sock);
l_extern l_integer l_socket_read(l_handle sock, void* out, l_integer count, l_integer* status);
l_extern l_integer l_socket_write(l_handle sock, const void* buf, l_integer count, l_integer* status);

l_extern void l_socket_test();
l_extern void l_plat_sock_test();

#endif /* l_socket_lib_h */

