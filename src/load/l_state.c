#include "lua.h"
#include "lauxlib.h"
#include "luacapi.h"
#include "service.h"

/** Continuations **
Through lua_pcall and lua_call, a C function called from Lua can call Lua back.
Several functions in the standard library do that: table.sort can call order
function; string.gsub can call a replacement function; pcall and xpcall call
functions in protected mode. */

lua_State* cclua_newstate() {
  lua_State* L = luaL_newstate();
  if (L == 0) {
    ccloge("luaL_newstate failed");
  }
  return L;
}

void cclua_close(lua_State* L) {
  if (L) lua_close(L);
}

static void ll_init_state(struct ccstate* co) {
  cczeron(co, sizeof(struct ccstate));
  ccsmplnode_init(&co->node);
  co->coref = LUA_NOREF;
}

nauty_bool ccstate_init(struct ccstate* ccco, lua_State* L, int (*func)(struct ccstate*), struct ccservice* srvc) {
  lua_State* co = 0;
  ll_init_state(ccco);
  if ((co = lua_newthread(L)) == 0) {
    ccloge("lua_newthread failed");
    return false;
  }
  ccco->L = L;
  ccco->co = co;
  ccco->coref = luaL_ref(L, LUA_REGISTRYINDEX);
  ccco->func = func;
  ccco->srvc = srvc;
  return true;
}

void ccstate_free(struct ccstate* co) {
  if (co->L && co->coref != LUA_NOREF) {
    /* if ref is LUA_NOREF or LUA_REFNIL, luaL_unref does nothing. */
    luaL_unref(co->L, LUA_REGISTRYINDEX, co->coref);
  }
}

/* return 0 OK, 1 YIELD, <0 CCTATUS_LUAERR or error code */
static int ll_state_resume(struct ccstate* co, int nargs) {
  /** lua_resume **
  int lua_resume(lua_State* L, lua_State* from, int nargs);
  Starts and resumes a coroutine in the given thread L.
  The parameter 'from' represents the coroutine that is resuming L.
  If there is no such coroutine, this parameter can be NULL.
  All arguments and the function value are popped from the stack when
  the function is called. The function results are pushed onto the
  stack when the function returns, and the last result is on the top
  of the stack. */
  int nelems = 0;
  int n = lua_resume(co->co, co->L, nargs);
  if (n == LUA_OK) {
    /* coroutine finishes its execution without errors, the stack in L contains
    all values returned by the coroutine main function 'func'. */
    if ((nelems = lua_gettop(co->co)) > 0) {
      lua_Integer nerror = lua_tointeger(co->co, -1); /* error code is on top */
      lua_pop(co->co, nelems);
      ccassert(nelems == 1);
      if (nerror < 0) {
        /* user program error code */
        return (int)nerror;
      }
    }
    return CCSTATUS_OK;
  }

  if (n == LUA_YIELD) {
    /* coroutine yields, the stack in L contains all values passed to lua_yield. */
    if ((nelems = lua_gettop(co->co)) > 0) {
      lua_Integer code = lua_tointeger(co->co, -1); /* the code is on top */
      lua_pop(co->co, nelems);
      ccassert(nelems == 1);
      if (code > 0) {
        return (int)code;
      }
    }
    return CCSTATUS_YIELD;
  }

  /* error code is returned and the stack in L contains the error object on the top.
  Lua itself only generates errors whose error object is a string, but programs may
  generate errors with any value as the error object. in case of errors, the stack
  is not unwound, so you can use the debug api over it.
  LUA_ERRRUN: a runtime error.
  LUA_ERRMEM: memory allocation error. For such errors, Lua does not call the message handler.
  LUA_ERRERR: error while running the message handler.
  LUA_ERRGCMM: error while running a __gc metamethod. For such errors, Lua does not call
  the message handler (as this kind of error typically has no relation with the function
  being called). */
  ccloge("lua_resume %s", lua_tostring(co->co, -1));
  lua_pop(co->co, lua_gettop(co->co)); /* pop elems exist including the error object */
  return CCSTATUS_LUAERR;
}

static int ll_state_func(lua_State* co) {
  int status = 0;
  struct ccstate* ccco = 0;
  ccco = (struct ccstate*)lua_touserdata(co, -1);
  lua_pop(co, 1);
  status = ccco->func(ccco);
  /* never goes here if ccco->func is yield inside */
  if (status < 0) {
    lua_pushinteger(co, status);
    return 1; /* return one result */
  }
  return 0;
}

int ccstate_resume(struct ccstate* co) {
  int nargs = 0;
  int costatus = 0;

  /** int lua_status(lua_State* L) **
  LUA_OK - start a new coroutine or restart it, or can call functions
  LUA_YIELD - can resume a suspended coroutine */
  if ((costatus = lua_status(co->co)) == LUA_OK) {
    /* start or restart coroutine, need to provide coroutine function */
    lua_pushcfunction(co->co, ll_state_func);
    lua_pushlightuserdata(co->co, co);
    return ll_state_resume(co, nargs+1);
  }

  if (costatus == LUA_YIELD) {
    /* no need to provide func again when coroutine is suspended */
    return ll_state_resume(co, 0);
  }

  ccloge("coroutine cannot be resumed");
  return CCSTATUS_LUAERR;
}

static int ll_state_kfunc(lua_State* co, int status, lua_KContext ctx) {
  struct ccstate* ccco = (struct ccstate*)ctx;
  (void)co; /* not used here, it should be equal to co->co */
  (void)status; /* status always is LUA_YIELD when kfunc is called after lua_yieldk */
  status = ccco->kfunc(ccco);
  /* never goes here if ccco->func is yield inside */
  if (status < 0) {
    lua_pushinteger(co, status);
    return 1; /* return one result */
  }
  return 0;
}

int ccstate_yieldwithcode(struct ccstate* co, int (*kfunc)(struct ccstate*), int code) {
  int status = 0;
  int nresults = 0;
  co->kfunc = kfunc;
  /* int lua_yieldk(lua_State* co, int nresults, lua_KContext ctx, lua_KFunction k);
  Usually, this function does not return; when the coroutine eventually resumes, it
  continues executing the continuation function. However, there is one special case,
  which is when this function is called from inside a line or a count hook. In that
  case, lua_yielkd should be called with no continuation (probably in the form of
  lua_yield) and no results, and the hook should return immediately after the call.
  Lua will yield and, when the coroutine resumes again, it will continue the normal
  execution of the (Lua) function that triggered the hook.
  This function can raise an error if it is called from a thread with a pending C
  call with no continuation function, or it is called from a thread that is not
  running inside a resume (e.g., the main thread). */
  if (code > 0) {
    lua_pushinteger(co->co, code);
    nresults += 1;
  }
  status = lua_yieldk(co->co, nresults, (lua_KContext)co, ll_state_kfunc);
  ccloge("lua_yieldk never returns to here"); /* the code never goes here */
  return status;
}

int ccstate_yield(struct ccstate* co, int (*kfunc)(struct ccstate*)) {
  return ccstate_yieldwithcode(co, kfunc, 0);
}

/** push stack functions **
void lua_pushcfunction(lua_State *L, lua_CFunction f);
void lua_pushinteger(lua_State *L, lua_Integer n);
void lua_pushlightuserdata(lua_State *L, void *p);
const char *lua_pushlstring(lua_State *L, const char *s, size_t len);
void lua_pushnumber(lua_State *L, lua_Number n);
const char *lua_pushstring(lua_State *L, const char *s);
const char *lua_pushliteral(lua_State *L, const char *s);
const char *lua_pushfstring(lua_State *L, const char *fmt, ...);
const char *lua_pushvfstring(lua_State *L, const char *fmt, va_list argp);
int lua_getfield(lua_State* L, int tblidx, const char* k); // push t[k], return pushed value type
int lua_gettable(lua_State* L, int tblidx); // push t[top_elem_as_key], top elem poped first
int lua_geti(lua_State* L, int tblidx, lua_Integer i); // push t[i], return value type
int lua_getglobal(lua_State* L, const char* name); // push global value, return value type
const char* lua_getupvalue(lua_State* L, int funcidx, int n); // push func's n-th upvalue, return name ("" for c func)
int lua_getuservalue(lua_State* L, int udidx); // push full userdata, return value type
 ** pop stack functions **
void lua_pop(lua_State* L, int n); // pops 'n' elements from the stack
void lua_remove(lua_State* L, int index); // removes the element at the given valid index
void lua_setfield(lua_State* L, int tblidx, const char* k); // set t[k] = top and pop top
void lua_settable(lua_State* L, int tblidx); // set t[top] = below top, and pop top and below top 
void lua_seti(lua_State* L, int tblidx, lua_Integer i); // set t[i] = top and pop top
void lua_setglobal(lua_State* L, const char* name); // set global = top and pop top
const char* lua_setupvalue(lua_State* L, int funcidx, int n); // set func's n-th upvalue = top and pop top
void lua_setuservalue(lua_State* L, int udidx); // set userdata = top and pop top 
 ** basic functions **
int lua_type(lua_State* L, int index); // LUA_TNONE/LUA_TNIL/LUA_TNUMBER/LUA_TBOOLEAN/LUA_TSTRING/LUA_TTABLE/LUA_TFUNCTION/
const char* lua_typename(lua_State* L, int type); // get name of the type     /LUA_TTHREAD/LUA_TUSERDATA/LUA_TLIGHTUSERDATA
int lua_gettop(lua_State* L); // top element's index, 0 means an empty stack
void lua_settop(lua_State* L, int index); // set top element's index, if larger new elems filled with nil, if 0 cleared
int lua_isboolean(lua_State* L, int index);
    lua_iscfunction/lua_isfunction "c or lua function"/lua_isinteger/lua_isnumber/lua_isstring
    lua_islightuserdata/lua_isuserdata "light or full userdata"/lua_istable/lua_isthread
    lua_isnil/lua_isnone/lua_isnoneornil "nil or invalid value"
int lua_toboolean(lua_State* L, int index); // return 0 if false or nil, otherwise return 1
lua_CFunction lua_tocfunction(lua_State* L, int index); // return a C function or NULL
lua_Integer lua_tointeger(lua_State* L, int index); // return integer or 0
lua_Integer lua_tointegerx(lua_State* L, int index, int* isnum); // return integer and isnum indicates success or not
lua_Number lua_tonumber(lua_State* L, int index); // return number or 0
lua_Number lua_tonumberx(lua_State* L, int index, int* isnum); // return number and isnum indicats success or not
const void* lua_topointer(lua_State* L, int index); // userdata/table/thread/function or NULL. it cann't convert back
const char* lua_tolstring(lua_State* L, int index, size_t* len); // get cstr and its length
const char* lua_tostring(lua_State* L, int index); // get cstr or NULL
lua_State* lua_tothread(lua_State* L, int index); // get thread or NULL
void* lua_touserdata(lua_State* L, int index); // get userdata or NULL */

static int ll_state_testfunc(struct ccstate* co) {
  static int i = 0;
  switch (i) {
  case 0:
    ++i;
    return ccstate_yield(co, ll_state_testfunc);
  case 1:
    ++i;
    return ccstate_yieldwithcode(co, ll_state_testfunc, 3);
  case 2:
    ++i;
    return ccstate_yield(co, ll_state_testfunc);
  case -1:
    --i;
    return ccstate_yieldwithcode(co, ll_state_testfunc, 5);
  case -2:
    --i;
    return ccstate_yield(co, ll_state_testfunc);
  case -3:
    i = 0;
    return -3;
  default:
    i = -1;
    break;
  }
  return 0;
}

static int ll_state_noyield(struct ccstate* co) {
  static int i = 0;
  (void)co;
  switch (i) {
  case 0:
    ++i;
    return 0;
  case 1:
    ++i;
    return -2;
  case 2:
    ++i;
    return -3;
  default:
    i = 0;
    break;
  }
  return 0;
}

void ccluatest() {
  lua_State* L = cclua_newstate();
  struct ccstate co;
  ccstate_init(&co, L, ll_state_testfunc, 0);
  ccassert(ccstate_resume(&co) == CCSTATUS_YIELD);
  ccassert(ccstate_resume(&co) == 3); /* yield */
  ccassert(ccstate_resume(&co) == CCSTATUS_YIELD);
  ccassert(ccstate_resume(&co) == 0);
  ccassert(ccstate_resume(&co) == 5); /* yield */
  ccassert(ccstate_resume(&co) == CCSTATUS_YIELD);
  ccassert(ccstate_resume(&co) == -3);
  co.func = ll_state_noyield;
  ccassert(ccstate_resume(&co) == 0);
  ccassert(ccstate_resume(&co) == -2);
  ccassert(ccstate_resume(&co) == -3);
  ccassert(ccstate_resume(&co) == 0);
  ccstate_free(&co);
  ccassert(sizeof(lua_KContext) >= sizeof(void*));
}
