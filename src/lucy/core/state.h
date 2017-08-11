#ifndef lucy_state_h
#define lucy_state_h

typedef struct l_state {
  L_COMMON_BUFHEAD
  l_thread* thread;
  lua_State* co;
  int coref;
  int (*func)(struct l_state*);
  int (*kfunc)(struct l_state*);
  l_service* srvc;
} l_state;

l_inline l_thread* l_state_belong(l_state* s) {
  return s->thread;
}

l_extern l_int lucy_intconf(lua_State* L, const void* name);
l_extern l_int lucy_intconf_n(lua_State* L, int n, ...);
l_extern int lucy_strconf(lua_State* L, int (*func)(void* stream, l_strt str), void* stream, const void* name);
l_extern int lucy_strconf_n(lua_State* L, int (*func)(void* stream, l_strt str), void* stream, int n, ...);

l_extern lua_State* l_new_luastate();
l_extern void l_close_luastate(lua_State* L);
l_extern int l_state_init(l_state* co, l_thread* thread, l_service* srvc, int (*func)(l_state*));
l_extern void l_state_free(l_state* co);

l_extern int l_state_is_yield(l_state* co);
l_extern int l_state_resume(l_state* co);
l_extern int l_state_yield(l_state* co, int (*kfunc)(l_state*));
l_extern int l_state_yield_with_code(l_state* co, int (*kfunc)(l_state*), int code);

#endif /* lucy_state_h */

