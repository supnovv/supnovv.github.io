#ifndef CCLIB_HTTP_SERVICE_H_
#define CCLIB_HTTP_SERVICE_H_
#include "service.h"
#include "socket.h"

#define CCM_HTTP_GET (0)
#define CCM_HTTP_HEAD (1)
#define CCM_HTTP_POST (2)

#define CCM_HTTP_VER_0NN (0)
#define CCM_HTTP_VER_10N (1)
#define CCM_HTTP_VER_11N (2)
#define CCM_HTTP_VER_2NN (3)

typedef struct {
  ccstring ip;
  ccshort_uint port;
  int (*response)(ccstate*);
  int backlog;
  ccmedit_uint rxsizelimit;
  cchandle_int sock;
} cchttplisten;

CORE_API int cchttp_listen(cchttplisten* self);

#endif /* CCLIB_HTTP_SERVICE_H_ */

