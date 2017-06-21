#ifndef CCLIB_HARBOR_SERVICE_H_
#define CCLIB_HARBOR_SERVICE_H_
#include "service.h"

CORE_API void ccharbor_sendmsg(struct ccservice* from, umedit_int destvsid, struct ccfrom ip, const void* msgdata, sright_int size);
CORE_API void ccharbor_sendmsgp(struct ccservice* from, umedit_int destvsid, const struct ccfrom* ip, const void* msgdata, sright_int size);

#endif /* CCLIB_HARBOR_SERVICE_H_ */

