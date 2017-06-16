#ifndef l_luacapi_lib_h
#define l_luacapi_lib_h
#include "thatcore.h"

l_extern lua_State* l_new_luastate();
l_extern void l_close_luastate(lua_State* L);

l_extern int l_state_init(l_state* co, l_thread* belong, l_service* srvc, int (*func)(l_state*));
l_extern void l_state_free(l_state* co);
l_extern int l_state_resume(l_state* co);
l_extern int l_state_yield(l_state* co, int (*kfunc)(l_state*));
l_extern int l_state_yield_with_code(l_state* co, int (*kfunc)(l_state*), int code);
l_extern void l_luac_test();

#endif /* l_luacapi_lib_h */

