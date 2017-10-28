#ifndef l_osi_platsock_h
#define l_osi_platsock_h
#include "core/prefix.h"

#if defined(l_plat_windows)
/** Windows Socket **/
#else
#include "osi/linuxpref.h"
/** POSIX Socket **/

/** Socket addresses **
struct sockaddr {
  sa_family_t sa_family;
  char        sa_data[14];
};
struct sockaddr_in {         // 'in' is for internet
  sa_family_t    sin_family; // address family: AF_INET
  in_port_t      sin_port;   // port in network byte-order
  struct in_addr sin_addr;   // internet address in network byte-order: struct in_addr { uint32_t s_addr; }
};
struct sockaddr_in6 {
    sa_family_t     sin6_family;   // AF_INET6
    in_port_t       sin6_port;     // port number
    uint32_t        sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // IPv6 address: struct in6_addr { unsigned char s6_addr[16]; }
    uint32_t        sin6_scope_id; // Scope ID (new in 2.4)
}; */

typedef union {
  struct sockaddr sa;
  struct sockaddr_in in;
  struct sockaddr_in6 in6;
} ll_sock_addr;

typedef struct {
  socklen_t len;
  ll_sock_addr addr;
} llsockaddr;

#ifdef L_CORE_AUTO_CONFIG
#define L_SOCKADDR_TYPE_SIZE sizeof(llsockaddr)
#endif

#endif
#endif /* l_osi_platsock_h */

