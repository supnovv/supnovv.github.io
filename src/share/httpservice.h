#ifndef l_http_lib_h
#define l_http_lib_h
#include "service.h"
#include "socket.h"

#define L_HTTP_GET (0)
#define L_HTTP_HEAD (1)
#define L_HTTP_POST (2)

#define L_HTTP_VER_0NN (0)
#define L_HTTP_VER_10N (1)
#define L_HTTP_VER_11N (2)
#define L_HTTP_VER_2NN (3)

l_extern int l_http_listen(l_from ip, int port, int (*rx)(l_state*));

#define l_service ccservice
#define l_message ccmessage
#define l_state ccstate

#define l_service_data ccservice_getdata
#define l_service_belong ccservice_belong

#define l_buffer_new ccnewbuffer
#define l_buffer_ensure_size_remain ccbuffer_ensuresizeremain

#define l_socket_read ccsocket_read

#define L_STATUS_EREAD CCSTATUS_EREAD
#define L_STATUS_ELIMIT CCSTATUS_ELIMIT
#define L_STATUS_WAITMORE CCSTATUS_WAITMORE
#define L_STATUS_CONTREAD CCSTATUS_CONTREAD

#endif /* l_http_lib_h */

