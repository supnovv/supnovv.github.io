#ifndef lucy_state_h
#define lucy_state_h
#include <lualib.h>
#include <lauxlib.h>
#include "core/base.h"

typedef struct {
  int index;
} l_funcindex;

typedef struct {
  int index;
} l_tableindex;

typedef struct l_luaextra l_luaextra;
typedef struct l_service l_service;

L_INLINE void
l_luastate_popError(lua_State* L)
{
  l_loge_1("lua error %s", ls(lua_tostring(L, -1)));
  lua_pop(L, 1);
}

L_EXTERN lua_State* l_luastate_new();
L_EXTERN void l_luastate_close(lua_State* L);
L_EXTERN void l_luastate_empty(lua_State* L); /* empty stack */

L_EXTERN l_funcindex l_luastate_load(lua_State* L, l_from from); /* push a function if success, otherwise unchanged */
L_EXTERN int l_luastate_call(lua_State* L, l_funcindex func, int nresults); /* pop args and func, and push nresults if success */
L_EXTERN int l_luastate_exec(lua_State* L, l_from from, int nresults); /* push nresults if success, otherwise unchanged */

L_EXTERN l_int l_luaconf_int(lua_State* L, const void* name);
L_EXTERN l_int l_luaconf_intx(lua_State* L, const void** s, l_int n);
L_EXTERN int l_luaconf_str(lua_State* L, const void* name, int (*read)(void* obj, l_strn s), void* obj);
L_EXTERN int l_luaconf_strx(lua_State* L, const void** s, l_int n, int (*read)(void* obj, l_strn s), void* obj);

L_EXTERN int l_service_initState(l_service* srvc);
L_EXTERN void l_service_freeState(l_service* srvc);
L_EXTERN int l_service_isYield(l_service* srvc);
L_EXTERN int l_service_setResume(l_service* srvc, int (*func)(l_service*));
L_EXTERN int l_service_resume(l_service* srvc);
L_EXTERN int l_service_yield(l_service* srvc, int (*kfunc)(l_service*));
L_EXTERN int l_service_yieldWithCode(l_service* srvc, int (*kfunc)(l_service*), int code);

#endif /* lucy_state_h */

