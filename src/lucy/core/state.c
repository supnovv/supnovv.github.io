#include <lualib.h>
#include <lauxlib.h>

#define L_LIBRARY_IMPL
#include "core/state.h"

/**
 * # Continuations
 *
 * Through lua_pcall and lua_call, a C function called from Lua can call Lua back.
 * Several functions in the standard library do that: table.sort can call order function;
 * string.gsub can call a replacement function; pcall and xpcall call functions in
 * protected mode. If we remember that the main Lua code was itself called from C (the
 * host program), we have a call sequence like C (host) calls Lua (script) that calls C
 * (library) that calls Lua (callback).
 *
 * Usually, Lua handles these sequences of calls without problems; after all, this integration
 * with C is a hallmark of the language. There is one situation, however, where this
 * interlacement can cause difficulties: coroutines.
 *
 * Each coroutine in Lua has its own stack, which keeps information about the pending calls of
 * the coroutine. Specifically, the stack stores the return address, the parameters, and the
 * local variables of each call. For calls to Lua functions, the interpreter needs only this
 * stack, which we call the soft stack. For calls to C functions, however, the interpreter must
 * use the C stack, too. After all, the return address and the local variables of a C function
 * live in the C stack.
 *
 * It is easy for the interpreter to have multiple soft stacks, but the runtime of ISO C has
 * only one internal stack. Therefore, coroutines in Lua cannot suspend the execution of a C
 * function: if there is a C function in the call path from a resume to its respective yield,
 * Lua cannot save the stage of that C function to restore it in the next resume.
 *
 * Consider the next example, in Lua 5.1:
 * ```
 * co = coroutine.wrap(function() print(pcall(coroutine.yield)) end)
 * co() -- false - attempt to yield across metamethod/C-call boundary
 * ```
 * The function pcall is a C function; therefore, Lua 5.1 cannot suspend it, because there is
 * no way in ISO C to suspend a C function and resume it later.
 *
 * Lua 5.2 and later versions ameliorated that difficulty with continuations. Lua 5.2 implements
 * yields using long jumps, in the same way that it implements errors. A long jump simply throws
 * away any information about C function in the C stack, so it is impossible to resume those
 * functions. However, a C function foo can specify a continuation function foo_k, which is
 * another C function to be called when it is time to resume foo. That is, when the interpreter
 * detects that it should resume foo, but that a long jump threw away the entry for foo in the
 * C stack, it calls foo_k instead.
 *
 * To make things a little more concrete, let us see the implementation of pcall as an example.
 * In Lua5.1, this function had the following code:
 * ```
 * static int luaB_pcall(lua_State* L) {
 *   int status;
 *   luaL_checkany(L, 1); // at least one parameter
 *   status = lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0);
 *   lua_pushboolean(L, (status == LUA_OK));
 *   lua_insert(L, 1); // status is first result
 *   return lua_gettop(L); // return status + all results
 * }
 * ```
 *
 * If the function being called through lua_pcall yielded, it would be impossible to resume
 * luaB_pcall later. Therefore, the interpreter raised an error whenever we attempted to yield
 * inside a protected call. Lua 5.3 implements pcall roughtly like below. There are three
 * important differences from the Lua 5.1 version: first, the new version replaces the call to
 * lua_pcall by a call to lua_pcallk; second, it puts everything done after that call in a new
 * auxiliary function finishpcall; third, the status returned by lua_pcallk can be LUA_YIELD,
 * besides LUA_OK or an error.
 * ```
 * static int finishpcall(lua_State* L, int status, intptr_t ctx) {
 *   (void)ctx; // unused parameter
 *   status = (status != LUA_OK && status != LUA_YIELD);
 *   lua_pushboolean(L, (status == 0));
 *   lua_insert(L, 1); // status is first result
 *   return lua_gettop(L); // return status + all results
 * }
 * static int luaB_pcall(lua_State* L) {
 *   int status;
 *   luaL_checkany(L, 1);
 *   status = lua_pcallk(L, lua_gettop(L) - 1, LUA_MULTRET, 0, 0, finishpcall);
 *   retrun finishpcall(L, status, 0);
 * }
 * ```
 *
 * If there are no yields, lua_pcallk works exactly like lua_pcall. If there is a yield,
 * however, then things are quite different. If a function called by the original lua_pcall
 * tries to yield, Lua 5.3 raises an error, like Lua 5.1. But when a function called by the
 * new lua_pcallk yields, there is no error: Lua does a long jump and discards the entry
 * for luaB_pcall from the C stack, but keeps in the soft stack of the coroutine a reference
 * to the continuation function given to lua_pcallk (finishpcall, in our example). Later,
 * when the interpreter detects that it should return to luaB_pcall (which is impossible),
 * it instead call the continuation function.
 *
 * The continuation function finishpcall can also be called when there is an error. Unlike
 * the original luaB_pcall, finishpcall cannot get the value returned by lua_pcallk. So,
 * it gets this value as an extra parameter, status. When there are no errors, status is
 * LUA_YIELD instead of LUA_OK, so that the continuation function can check how it is
 * being called. In case of errors, status is the original error code.
 *
 * Besides the status of the call, the continuation function also receives a context. The
 * fifth parameter to lua_pcallk is an arbitrary integer that is passed as the last paramter
 * to the continuation function. (The type of this paramter, intptr_t, allows pointers to be
 * passed as context, too.) This value allows the original function to pass some arbitrary
 * information directly to its continuation. (Our example does not use this facility.)
 *
 * The continuation system of Lua 5.3 is an ingenious mechanism to support yields, but it is
 * not a panacea. Some C functions would need to pass too much context to their continuations.
 * Example include table.sort, which uses the C stack for recursion, adn string.gsub, which
 * must keep track of captures and a buffer for its partial result. Although it is possible
 * to rewrite them in a "yieldable" way, the gains do not seem to be worth the extra complexity
 * and performance losses.
 *
 */

static void
lucy_perror(lua_State* L, const void* log)
{
  if (log != 0) l_loge_2("%s - %s", ls(log), ls(lua_tostring(L, /* stack index */ -1)));
  lua_pop(L, /* number of elements */ 1); /* pop error object */
}

static int
lucy_loadfile(lua_State* L, const void* file)
{
  if (luaL_loadfile(L, (const char*)file) != LUA_OK) {
    lucy_perror(L, "lua_loadfile");
    return false;
  }
  return true;
}

static int
lucy_pcall(lua_State* L, int nresults)
{
  if (lua_pcall(L, /* nargs */ 0, nresults, /* msgh */ 0) != LUA_OK) {
    lucy_perror(L, "lua_pcall");
    return false;
  }
  return true;
}

static int /* execute file and nresults pushed onto stack if success */
lucy_dofile(lua_State* L, const void* file, int nresults)
{
  if (!lucy_loadfile(L, file)) return false;
  if (!lucy_pcall(L, nresults < 0 ? 0 : nresults)) return false;
  return true;
}

static int
lucy_setfuncenv(lua_State* L, const void* tablename)
{
  int funcindex = lua_gettop(L);
  lua_getglobal(L, (const char*)tablename); /* push a table as a upvalue */
  lua_setupvalue(L, funcindex, /* upvalue index - _ENV is the first upvalue */ 1); /* pop the table */
  return true;
}

static int
lucy_settablefield(lua_State* L, const void* keyname)
{
  int tableindex = lua_gettop(L) - 1;
  lua_setfield(L, tableindex, (const char*)keyname); /* pop the top value */
  return true;
}

static void
lucy_emptystack(lua_State* L)
{
  lua_pop(L, lua_gettop(L));
}

#define L_MAX_CONF_NAME_LEN 80

static int
lucy_loadconf(lua_State* L, const l_byte* name)
{
  const char* libname = "LUCY_GLOBAL_TABLE";
  l_byte keyname[L_MAX_CONF_NAME_LEN+1] = {0};
  l_byte* keyend = 0;
  l_int len = 0;

  if (!name || name[0] == 0) {
    return false;
  }

  lua_getglobal(L, libname); /* push the table */

  for (; ;) {
    keyend = keyname;
    len = 0;

    while (*name) {
      if (*name == '.') {
        name += 1;
        break;
      }
      *keyend++ = *name++;
      len = keyend - keyname;
      if (len == L_MAX_CONF_NAME_LEN) {
        l_loge_s("loadconf keyname too long");
        len = 0; /* set invalid len value */
        break;
      }
    }

    *keyend = 0; /* zero-terminated char */

    if (len == 0) {
      l_loge_s("loadconf invalid keyname");
      return false;
    }

    if (!lua_istable(L, -1)) {
      l_loge_1("loadconf %s from not a table", ls(keyname));
      return false;
    }

    lua_getfield(L, /* table index */ -1, (const char*)keyname); /* push the field value */

    if (*name == 0) {
      break;
    }
  }

  return true;
}

L_EXTERN l_int
lucy_intconf(lua_State* L, const void* name)
{
  int startelems = 0;
  l_int result = 0;

  startelems = lua_gettop(L);

  if (!lucy_loadconf(L, (const l_byte*)name)) {
    lua_pop(L, lua_gettop(L) - startelems);
    l_logw_s("load int conf failed");
    return 0;
  }

  result = lua_tointeger(L, -1);
  lua_pop(L, lua_gettop(L) - startelems);
  return result;
}

L_EXTERN int
lucy_strconf(lua_State* L, int (*func)(void* stream, l_strt str), void* stream, const void* name)
{
  int startelems = 0;
  const char* result = 0;
  size_t len = 0;

  startelems = lua_gettop(L);

  if (!lucy_loadconf(L, (const l_byte*)name)) {
    lua_pop(L, lua_gettop(L) - startelems);
    l_logw_s("load str conf failed");
    return false;
  }

  /* const char* lua_tolstring(lua_State* L, int index, size_t* len);
   * The Lua value must be a string or a number; otherwise, the function returns NULL. */
  result = lua_tolstring(L, -1, &len);
  if (!result) {
    return false;
  }

  return func(stream, l_strt_e(result, result + len));
}

static int
lucy_loadconf_n(lua_State* L, int n, va_list vl)
{
  const char* libname = "LUCY_GLOBAL_TABLE";
  const char* keyname = 0;
  if (n <= 0) return false;
  lua_getglobal(L, libname); /* push the table */
  while (n-- > 0) {
    keyname = va_arg(vl, const char*);
    if (keyname == 0) {
      l_loge_s("loadconf invalid keyname");
      return false;
    }
    if (!lua_istable(L, -1)) {
      l_loge_1("loadconf %s from not a table", ls(keyname));
      return false;
    }
    lua_getfield(L, /* table index */ -1, keyname); /* push the field value */
  }
  return true;
}

L_EXTERN l_int
lucy_intconf_n(lua_State* L, int n, ...)
{
  int startelems = 0;
  l_int result = 0;
  va_list vl;
  va_start(vl, n);
  startelems = lua_gettop(L);

  if (!lucy_loadconf_n(L, n, vl)) {
    lua_pop(L, lua_gettop(L) - startelems);
    l_logw_s("load int conf failed");
    va_end(vl);
    return 0;
  }

  result = lua_tointeger(L, -1);
  lua_pop(L, lua_gettop(L) - startelems);
  va_end(vl);
  return result;
}

L_EXTERN int
lucy_strconf_n(lua_State* L, int (*func)(void* stream, l_strt str), void* stream, int n, ...)
{
  int startelems = 0;
  const char* result = 0;
  size_t len = 0;
  va_list vl;
  va_start(vl, n);
  startelems = lua_gettop(L);

  if (!lucy_loadconf_n(L, n, vl)) {
    lua_pop(L, lua_gettop(L) - startelems);
    l_logw_s("load str conf failed");
    va_end(vl);
    return false;
  }

  va_end(vl);

  result = lua_tolstring(L, -1, &len);
  if (!result) {
    return false;
  }

  return func(stream, l_strt_e(result, result + len));
}

static void
l_init_luastate(lua_State* L)
{
  const char* libname = "LUCY_GLOBAL_TABLE";

  /* open all standard lua libraries first */
  luaL_openlibs(L);

  lua_newtable(L); /* push a empty table */
  lua_pushliteral(L, L_ROOT_DIR); /* push a string */
  lucy_settablefield(L, "rootdir"); /* pop the string */
  lua_setglobal(L, libname); /* pop the table */

  if (!lucy_dofile(L, L_ROOT_DIR "conf/init.lua", 0)) {
    l_loge_s("execute init.lua failed");
    return;
  }

  if (!lucy_loadfile(L, L_ROOT_DIR "conf/conf.lua")) { /* push loaded function */
    l_loge_s("load conf.lua failed");
    return;
  }

  if (!lucy_setfuncenv(L, libname)) {
    l_loge_s("set conf.lua _ENV failed ");
    return;
  }

  if (!lucy_pcall(L, 0)) { /* pop the function */
    l_loge_s("execute conf.lua failed");
    return;
  }

#if 0
  lua_getglobal(L, libname); /* push the table */
  if (!lucy_dofile(L, L_ROOT_DIR "core/base.lua", 1)) { /* push one result */
    l_loge_s("open base.lua failed");
    lua_pop(L, 1); /* pop the table */
    return;
  }
  lucy_settablefield(L, "base"); /* pop the reult */
  lua_pop(L, 1); /* pop the table */
#endif
}

L_EXTERN lua_State*
l_new_luastate()
{
  lua_State* L = luaL_newstate();
  if (L == 0) {
    l_loge_s("luaL_newstate failed");
    return 0;
  }
  l_init_luastate(L);
  return L;
}

L_EXTERN void
l_close_luastate(lua_State* L)
{
  if (L) lua_close(L);
}

L_EXTERN int
l_service_init_state(l_service* srvc)
{
  lua_State* L = srvc->thread->L;
  srvc->coref = LUA_NOREF;
  /**
   * lua_State* lua_newthread(lua_State* L);
   *
   * Creates a new thread, pushes it on the stack, and returns a ponter to
   * lua_State that represents this new thread. The new thread returned by
   * this function shares with the original thread its global environment,
   * but has an independent execution stack.
   *
   * There is no explicit function to close or to destroy a thread. Threads
   * are subject to garbage collection, like any Lua object.
   *
   */
  if (!(srvc->co = lua_newthread(L))) {
    l_loge_s("lua_newthread failed");
    return false;
  }
  /**
   * int luaL_ref(lua_State* L, int t);
   *
   * Creates and returns a reference, in the table at index t, for the object
   * at the top of the stack (and pops the object).
   *
   * A reference is a unique integer key. As long as you do not manually add
   * integer keys into table t, luaL_ref ensures the uniqueness of the key it
   * returns. You can retrieve an object referred by reference r by calling
   * lua_rawgeti(L, t, r). Function luaL_unref frees a reference and its
   * associated object.
   *
   * If the object at the top of the stack is nil, luaL_ref returns the
   * constant LUA_REFNIL. The constant LUA_NOREF is guaranteed to be different
   * from any reference returned by luaL_ref.
   *
   */
  srvc->coref = luaL_ref(L, LUA_REGISTRYINDEX);
  return true;
}

L_EXTERN void
l_service_free_state(l_service* srvc)
{
  if (srvc->coref == LUA_NOREF) return;
  luaL_unref(srvc->thread->L, LUA_REGISTRYINDEX, srvc->coref); /* do nothing if LUA_NOREF/LUA_REFNIL */
  srvc->co = 0;
  srvc->coref = LUA_NOREF;
}

static int /* return 0 OK, 1 YIELD, <0 L_STATUS_LUAERR or error code */
llstateresume(l_service* srvc, int nargs)
{
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
  int n = lua_resume(srvc->co, srvc->thread->L, nargs);
  if (n == LUA_OK) {
    /* coroutine finishes its execution without errors, the stack in L contains
    all values returned by the coroutine main function 'func'. */
    if ((nelems = lua_gettop(srvc->co)) > 0) {
      lua_Integer nerror = lua_tointeger(srvc->co, -1); /* error code is on top */
      lua_pop(srvc->co, nelems);
      l_assert(nelems == 1);
      if (nerror < 0) {
        /* user program error code */
        return (int)nerror;
      }
    }
    return L_STATUS_OK;
  }

  if (n == LUA_YIELD) {
    /* coroutine yields, the stack in L contains all values passed to lua_yield. */
    if ((nelems = lua_gettop(srvc->co)) > 0) {
      lua_Integer code = lua_tointeger(srvc->co, -1); /* the code is on top */
      lua_pop(srvc->co, nelems);
      l_assert(nelems == 1);
      if (code > 0) {
        return (int)code;
      }
    }
    return L_STATUS_YIELD;
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
  l_loge_1("lua_resume %s", ls(lua_tostring(srvc->co, -1)));
  lua_pop(srvc->co, lua_gettop(srvc->co)); /* pop elems exist including the error object */
  return L_STATUS_LUAERR;
}

static int
llstatefunc(lua_State* co)
{
  int status = 0;
  l_service* srvc = 0;
  srvc = (l_service*)lua_touserdata(co, -1);
  lua_pop(srvc->co, 1);
  status = srvc->func(srvc);
  /* never goes here if ccco->func is yield inside */
  if (status < 0) {
    lua_pushinteger(srvc->co, status);
    return 1; /* return one result */
  }
  return 0;
}

L_EXTERN int
l_service_is_yield(l_service* srvc)
{
  /**
   * int lua_status(lua_State* L);
   *
   * Returns the status of the thread L. The status can be 0 (LUA_OK) for a normal thread,
   * an error code if the thread finished the execution of a lua_resume with an error, or
   * LUA_YIELD if the thread is suspended.
   *
   * You can only call functions in threads with status LUA_OK. You can resume threads with
   * status LUA_OK (to start a new coroutine) or LUA_YIELD (to resume a coroutine).
   *
   */
  return lua_status(srvc->co) == LUA_YIELD;
}

L_EXTERN int
l_service_is_luaok(l_service* srvc)
{
  return lua_status(srvc->co) == LUA_OK;
}

L_EXTERN int
l_service_set_resume(l_service* srvc, int (*func)(l_service*))
{
  int status = 0;

  if (srvc->co == 0) {
    l_service_init_state(srvc);
  }

  srvc->func = func;

  if ((status = lua_status(srvc->co)) != LUA_OK) {
    l_loge_1("lua_State is not in LUA_OK status %d", ld(status));
    return false;
  }

  return true;
}

L_EXTERN int
l_service_resume(l_service* srvc)
{
  int nargs = 0;
  int costatus = 0;

  /** int lua_status(lua_State* L) **
  LUA_OK - start a new coroutine or restart it, or can call functions
  LUA_YIELD - can resume a suspended coroutine */
  if ((costatus = lua_status(srvc->co)) == LUA_OK) {
    /* start or restart coroutine, need to provide coroutine function */
    lua_pushcfunction(srvc->co, llstatefunc);
    lua_pushlightuserdata(srvc->co, srvc);
    return llstateresume(srvc, nargs+1);
  }

  if (costatus == LUA_YIELD) {
    /* no need to provide func again when coroutine is suspended */
    return llstateresume(srvc, 0);
  }

  l_loge_s("coroutine cannot be resumed");
  return L_STATUS_LUAERR;
}

static int
llstatekfunc(lua_State* co, int status, lua_KContext ctx)
{
  l_service* srvc = (l_service*)ctx;
  l_assert(co == srvc->co); /* co passed here should be equal to srvc->co */
  (void)status; /* status always is LUA_YIELD when kfunc is called after lua_yieldk */
  status = srvc->kfunc(srvc);
  /* never goes here if srvc->func is yield inside */
  if (status < 0) {
    lua_pushinteger(co, status);
    return 1; /* return one result */
  }
  return 0;
}

L_EXTERN int
l_service_yield_with_code(l_service* srvc, int (*kfunc)(l_service*), int code)
{
  int status = 0;
  int nresults = 0;
  srvc->kfunc = kfunc;
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
    lua_pushinteger(srvc->co, code);
    nresults += 1;
  }
  status = lua_yieldk(srvc->co, nresults, (lua_KContext)srvc, llstatekfunc);
  l_loge_s("lua_yieldk never returns to here"); /* the code never goes here */
  return status;
}

L_EXTERN int
l_service_yield(l_service* srvc, int (*kfunc)(l_service*))
{
  return l_service_yield_with_code(srvc, kfunc, 0);
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

static int
llstatetestfunc(l_service* srvc)
{
  static int i = 0;
  switch (i) {
  case 0:
    ++i;
    return l_service_yield(srvc, llstatetestfunc);
  case 1:
    ++i;
    return l_service_yield_with_code(srvc, llstatetestfunc, 3);
  case 2:
    ++i;
    return l_service_yield(srvc, llstatetestfunc);
  case -1:
    --i;
    return l_service_yield_with_code(srvc, llstatetestfunc, 5);
  case -2:
    --i;
    return l_service_yield(srvc, llstatetestfunc);
  case -3:
    i = 0;
    return -3;
  default:
    i = -1;
    break;
  }
  return 0;
}

static int
llstatenoyield(l_service* srvc)
{
  static int i = 0;
  (void)srvc;
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

L_EXTERN void
l_luac_test()
{
  lua_State* L = l_new_luastate();
  l_service srvc;
  l_thread thread;
  srvc.thread = &thread;
  thread.L = L;
  l_service_init_state(&srvc);

  l_service_set_resume(&srvc, llstatetestfunc);
  l_assert(l_service_resume(&srvc) == L_STATUS_YIELD);
  l_assert(l_service_resume(&srvc) == 3); /* yield */
  l_assert(l_service_resume(&srvc) == L_STATUS_YIELD);
  l_assert(l_service_resume(&srvc) == 0);
  l_assert(l_service_resume(&srvc) == 5); /* yield */
  l_assert(l_service_resume(&srvc) == L_STATUS_YIELD);
  l_assert(l_service_resume(&srvc) == -3);

  l_service_set_resume(&srvc, llstatenoyield);
  l_assert(l_service_resume(&srvc) == 0);
  l_assert(l_service_resume(&srvc) == -2);
  l_assert(l_service_resume(&srvc) == -3);
  l_assert(l_service_resume(&srvc) == 0);

  l_assert(sizeof(lua_KContext) >= sizeof(void*));

  l_assert(lucy_intconf(L, "test.a") == 10);
  l_assert(lucy_intconf(L, "test.t.b") == 20);
  l_assert(lucy_intconf(L, "test.t.c") == 30);
  l_assert(lucy_intconf(L, "test.d") == 40);

  /* srvc->co shared with L's global environment */
  l_assert(lucy_intconf(srvc.co, "test.a") == 10);
  l_assert(lucy_intconf(srvc.co, "test.t.b") == 20);
  l_assert(lucy_intconf(srvc.co, "test.t.c") == 30);
  l_assert(lucy_intconf(srvc.co, "test.d") == 40);

  l_service_free_state(&srvc);
}

