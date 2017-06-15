#ifndef l_luacapi_lib_h
#define l_luacapi_lib_h
#include "thatcore.h"

typedef struct lua_State lua_State;
l_extern lua_State* l_new_luastate();
l_extern void l_close_luastate(lua_State* L);

typedef struct l_service l_service;
typedef struct l_message l_message;

typedef struct l_state {
  l_smplnode node;
  lua_State* L;
  lua_State* co;
  int coref;
  int (*func)(struct l_state*);
  int (*kfunc)(struct l_state*);
  l_service* srvc;
  l_message* msg;
} l_state;

l_extern int l_state_init(l_state* co, lua_State* L, int (*func)(l_state*), l_service* srvc);
l_extern void l_state_free(l_state* co);
l_extern int l_state_resume(l_state* co);
l_extern int l_state_yield(l_state* co, int (*kfunc)(l_state*));
l_extern int l_state_yield_with_code(l_state* co, int (*kfunc)(l_state*), int code);
l_extern void l_lua_test();

#endif /* l_luacapi_lib_h */

