#ifndef CCLIB_SOCKET_H_
#define CCLIB_SOCKET_H_
#include "thatcore.h"

#define CCBACKLOG 32

struct ccsockaddr {
  CCPLAT_IMPL_SIZE(CC_SOCKADDR_BYTES);
};

struct ccsocket {
  handle_int id;
};

struct ccsockconn {
  struct ccsocket sock;
  struct ccsockaddr remote;
};

CORE_API nauty_bool ccsockaddr_init(struct ccsockaddr* self, struct ccfrom ip, ushort_int port);
CORE_API nauty_bool ccsockaddr_initp(struct ccsockaddr* self, const struct ccfrom* ip, ushort_int port);
CORE_API ushort_int ccsockaddr_getport(struct ccsockaddr* self);
CORE_API nauty_bool ccsockaddr_getipstr(struct ccsockaddr* self, struct ccstring* out);

CORE_API struct ccsocket ccsocket_listen(const struct ccsockaddr* addr, int backlog);
CORE_API void ccsocket_accept(struct ccsocket* sock, int (*cb)(struct ccsockconn*));
CORE_API void ccsocket_close(struct ccsocket* sock);
CORE_API void ccsocket_shutdown(struct ccsocket* sock, nauty_char r_w_a);
CORE_API void ccsocketconn_init(struct ccsockconn* self, struct ccfrom ip, ushort_int port);

CORE_API nauty_bool ccsocket_connect(struct ccsockconn* conn);
CORE_API nauty_bool ccsocket_isopen(const struct ccsocket* sock);
CORE_API struct ccsockaddr ccsocket_getlocaladdr(struct ccsocket* sock);
CORE_API sright_int ccsocket_read(const struct ccsockconn* conn, void* out, sright_int count);
CORE_API sright_int ccsocket_write(const struct ccsockconn* conn, const void* buf, sright_int count);

CORE_API void ccsockettest();
CORE_API void ccplatsocktest();

#endif /* CCLIB_SOCKET_H_ */

