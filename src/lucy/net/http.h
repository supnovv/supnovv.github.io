#ifndef lucy_net_http_h
#define lucy_net_http_h
#include "lucycore.h"

l_extern int l_start_http_server(l_strt ip, l_ushort port, int (*client_request_handler)(l_state*));


#endif /* lucy_net_http_h */

