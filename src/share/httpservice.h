#ifndef l_http_lib_h
#define l_http_lib_h
#include "l_service.h"
#include "l_socket.h"

l_extern int l_http_listen(l_from ip, int port, int (*rx)(l_state*));


#endif /* l_http_lib_h */

