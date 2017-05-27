#ifndef CCLIB_SOCKET_H_
#define CCLIB_SOCKET_H_
#include "thatcore.h"

#define SOCKET_BACKLOG  (32)
#define SOCKET_IPSTRLEN (48)

struct ccsockaddr {
  CCPLAT_IMPL_SIZE(CC_SOCKADDR_BYTES);
};

struct ccsockconn {
  handle_int sock;
  struct ccsockaddr remote;
};

CORE_API nauty_bool ccsockaddr_init(struct ccsockaddr* self, struct ccfrom ip, ushort_int port);
CORE_API nauty_bool ccsockaddr_initp(struct ccsockaddr* self, const struct ccfrom* ip, ushort_int port);
CORE_API ushort_int ccsockaddr_getport(struct ccsockaddr* self);
CORE_API nauty_bool ccsockaddr_getipstr(struct ccsockaddr* self, struct ccstring* out);

CORE_API handle_int ccsocket_listen(const struct ccsockaddr* addr, int backlog);
CORE_API void ccsocket_accept(handle_int sock, void (*cb)(void*, struct ccsockconn*), void* ud);
CORE_API void ccsocket_close(handle_int sock);
CORE_API void ccsocket_shutdown(handle_int sock, nauty_char r_w_a);
CORE_API void ccsocketconn_init(struct ccsockconn* self, struct ccfrom ip, ushort_int port);

CORE_API nauty_bool ccsocket_connect(struct ccsockconn* conn);
CORE_API nauty_bool ccsocket_isopen(handle_int sock);
CORE_API struct ccsockaddr ccsocket_getlocaladdr(handle_int sock);
CORE_API sright_int ccsocket_read(handle_int sock, void* out, sright_int count);
CORE_API sright_int ccsocket_write(handle_int sock, const void* buf, sright_int count);

CORE_API void ccsockettest();
CORE_API void ccplatsocktest();

#endif /* CCLIB_SOCKET_H_ */

