#ifndef l_master_lib_h
#define l_master_lib_h
#include "thatcore.h"

l_extern int startmainthread(int (*start)());
l_extern int startmainthreadcv(int (*start)(), int argc, char** argv);

#endif /* l_master_lib_h */

