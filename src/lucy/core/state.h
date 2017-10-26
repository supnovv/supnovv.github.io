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
L_EXTERN void l_luastate_empty(lua_State* L); /* empty the stack */
L_EXTERN l_funcindex l_luastate_load(lua_State* L, l_from from); /* [-0, +(1|0), -] */
L_EXTERN int l_luastate_call(lua_State* L, l_funcindex func, int nresults); /* [-(nargs+1), +(nresults|0), -] */
L_EXTERN int l_luastate_exec(lua_State* L, l_from from, int nresults); /* [-0, +(nresults|0), -] */
L_EXTERN void l_luastate_setenv(lua_State* L, l_funcindex func); /* the env table is on the top of the stack [-1, +0, -] */
L_EXTERN void l_luastate_setfield(lua_State* L, l_tableindex table, const void* keyname); /* the value is on the top of the stack [-1, +0, e] */
L_EXTERN l_tableindex l_luastate_newtable(lua_State* L);

L_EXTERN l_int l_luaconf_int(lua_State* L, const void* name);
L_EXTERN l_int l_luaconf_intv(lua_State* L, int n, ...);
L_EXTERN int l_luaconf_str(lua_State* L, int (*read)(void* obj, l_strn s), void* obj, const void* name);
L_EXTERN int l_luaconf_strv(lua_State* L, int (*read)(void* obj, l_strn s), void* obj, int n, ...);

#endif /* lucy_state_h */

