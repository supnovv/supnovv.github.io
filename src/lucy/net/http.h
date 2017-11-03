#ifndef l_net_http_h
#define l_net_http_h
#include "core/base.h"

L_EXTERN int l_httpd_start(const void* http_conf_name, int (*client_request_handler)(l_service*));


#endif /* l_net_http_h */

