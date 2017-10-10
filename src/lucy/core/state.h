#ifndef lucy_state_h
#define lucy_state_h
#include "core/base.h"

typedef struct lua_State lua_State;
L_EXTERN l_int lucy_intconf(lua_State* L, const void* name);
L_EXTERN l_int lucy_intconf_n(lua_State* L, int n, ...);
L_EXTERN int lucy_strconf(lua_State* L, int (*func)(void* stream, l_strt str), void* stream, const void* name);
L_EXTERN int lucy_strconf_n(lua_State* L, int (*func)(void* stream, l_strt str), void* stream, int n, ...);

L_EXTERN lua_State* l_new_luastate();
L_EXTERN void l_close_luastate(lua_State* L);

L_EXTERN int l_service_init_state(l_service* srvc);
L_EXTERN void l_service_free_state(l_service* srvc);
L_EXTERN int l_service_is_luaok(l_service* srvc);
L_EXTERN int l_service_is_yield(l_service* srvc);
L_EXTERN int l_service_set_resume(l_service* srvc, int (*func)(l_service*));
L_EXTERN int l_service_resume(l_service* srvc);
L_EXTERN int l_service_yield(l_service* srvc, int (*kfunc)(l_service*));
L_EXTERN int l_service_yield_with_code(l_service* srvc, int (*kfunc)(l_service*), int code);

#endif /* lucy_state_h */

