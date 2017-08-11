#ifndef lucy_net_http_h
#define lucy_net_http_h
#include "lucycore.h"

l_extern int l_start_http_server(const void* http_conf_name, int (*client_request_handler)(l_state*));


#endif /* lucy_net_http_h */

