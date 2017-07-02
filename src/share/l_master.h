#ifndef l_master_lib_h
#define l_master_lib_h
#include "thatcore.h"

l_extern int startmainthread(int (*start)());
l_extern int startmainthreadcv(int (*start)(), int argc, char** argv);
l_extern void l_master_exit();

#endif /* l_master_lib_h */

