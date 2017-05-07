#ifndef CCLIB_SOCKET_H_
#define CCLIB_SOCKET_H_
#include "thatcore.h"

struct ccsockaddr {
  CCPLAT_IMPL_SIZE(CC_SOCKADDR_BYTES);
};

CORE_API nauty_bool ccsockaddr_init(struct ccsockaddr* self, struct ccfrom ip, ushort_int port);
CORE_API nauty_bool ccsockaddr_initp(struct ccsockaddr* self, const struct ccfrom* ip, ushort_int port);
CORE_API ushort_int ccsockaddr_getport(struct ccsockaddr* self);
CORE_API nauty_bool ccsockaddr_getipstr(struct ccsockaddr* self, struct ccstring* out);

typedef handle_int socket_int;

CORE_API nauty_bool ccsocket_bind(socket_int sock, struct ccfrom ip, ushort_int port);
CORE_API struct ccsockaddr ccsocket_getboundaddr(socket_int sock);
CORE_API nauty_bool ccsocket_listen(socket_int sock, int backlog);

CORE_API void ccsockettest();
CORE_API void ccplatsocktest();

#endif /* CCLIB_SOCKET_H_ */

