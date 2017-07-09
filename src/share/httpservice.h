#ifndef l_http_lib_h
#define l_http_lib_h
#include "thatcore.h"
#include "l_state.h"

l_extern int l_start_http_server(l_strt ip, l_ushort port, int (*client_request_handler)(l_state*));


#endif /* l_http_lib_h */

