#define L_LIBRARY_IMPL
#include "core/state.h"

typedef struct l_luaextra {

} l_luaextra;

static void l_luaconf_init(lua_State* L);

static void
l_luaextra_init(lua_State* L)
{
  l_luaextra** extra = (l_luaextra**)lua_getextraspace(L);
  *extra = (l_luaextra*)l_raw_malloc(sizeof(l_luaextra));
  /* TODO */
}

static void
l_luaextra_free(lua_State* L)
{
  l_luaextra** extra = (l_luaextra**)lua_getextraspace(L);
  if (*extra) {
    l_raw_mfree(*extra);
    *extra = 0;
  }
}

L_EXTERN l_luaextra*
l_luaextra_get(lua_State* L)
{
  return *((l_luaextra**)lua_getextraspace(L));
}

L_EXTERN lua_State*
l_luastate_new()
{
  lua_State* L;

  if (!(L = luaL_newstate())) {
    l_loge_s("luaL_newstate failed");
    return 0;
  }

  luaL_openlibs(L); /* open all standard lus libraries */
  l_luaextra_init(L);
  l_luaconf_init(L);
  return L;
}

L_EXTERN void
l_luastate_close(lua_State* L)
{
  if (!L) return;
  l_luaextra_free(L);
  lua_close(L);
}

L_EXTERN void
l_luastate_empty(lua_State* L)
{
  lua_pop(L, lua_gettop(L));
}

/** Lua's C API function indicator [-o, +p, x]
The first field, o, is how many elements the function pops from the stack.
The second field, p, is how many elements the function pushes onto the stack.
Any function always pushes its results after popping its arguments.
A field in the form x|y means the function can push (or pop) x or y elements, depending on the situation.
The question mark '?' means that we cannot know how many elements the function pops/pushes by looking only at its args.
The third field, x, tells whether the function may raise errors.
The '-' means the function never raises any error.
The 'm' means the function may raise out-of-memory errors and errors running a __gc metamethod.
The 'e' means the function may raise any errors (it can run arbitrary Lua code, either directly or through metamethods).
The 'v' means the function may raise an error on purpose.
*/

/** load function
typedef const char* (*lua_Reader)(lua_State* L, void* data, size_t* size);
int lua_load(lua_State* L, lua_Reader reader, void* data, const char* chunkname, const char* mode); [-0, +1, -]
int luaL_loadstring(lua_State* L, const char* s); [-0, +1, -]
int luaL_loadbuffer(lua_State* L, const char* buffer, size_t sz, const char* name); [-0, +1, -]
int luaL_loadbufferx(lua_State* L, const char* buffer, size_t sz, const char* name, const char* mode); [-0, +1, -]
int luaL_loadfile(lua_State* L, const char* filename); [-0, +1, m]
int luaL_loadfilex(lua_State* L, const char* filename, const char* mode); [-0, +1, m]
---
## lua_Reader
Every time it needs another piece of the chunk, lua_load calls the reader, passing along its data paramter.
The reader must return a pointer to a block of memory with a new piece of the chunk and set size to the block size.
The block must exist nutil the reader function is called again.
To signal the end of the chunk, the reader must return NULL or set size to zero.
The reader function may return pieces of any size greater than zero.
The reader function must always leave the stack unmodified when returning.
---
## lua_load(L, reader, data, chunkname, mode) -> [pop 0, push 1 result, raise 0 error]
Loads a Lua chunk without running it.
If there are no errors, lua_load pushes the compiled chunk as a Lua function on top of stack.
Otherwise, it pushes an error message.
The return values are: LUA_OK (no errors), LUA_ERRSYNTAX, LUA_ERRMEM, LUA_ERRGCMM.
The lua_load uses a user-supplied reader function to read the chunk.
The data argument is an opaque value passed to the reader function.
The chunkname argument gives a name to the chunk, which is used for error messages and in debug information.
If chunkname is NULL, it is equivalent to the string "?": `if (!chunkname) chunkname = "?";`
The lua_load automatically detects whether the chunk is text or binary and loads it accordingly.
The string mode works as in function load, with the addition that a NULL value is equivalent to the string "bt".
The string mode controls whether the chunk can be text or binary (precompiled chunk).
It may be "b" (only binary chunks), "t" (only text chunks), or "bt" (both binary and text).
Lua does not check the consistency of binary chuncks. Maliciously crafted binary chunks can crash the interpreter.
The lua_load uses the stack internally, so the reader function must always leave the stack unmodified when returning.
If the resulting function has upvalues, its first upvalue is set to the value of the global environment.
Other upvalues are initialized with nil.
---
## luaL_loadstring(L, s) -> [pop 0, push 1 result, raise 0 error]
This function uses lua_load to load the chunk in the zero-terminated string s.
This function returns the same results as lua_load.
---
## luaL_loadbufferx(L, buffer, size, name, mode) -> [pop 0, push 1 result, raise 0 error]
This function uses lua_load to load the chunk in the buffer pointed by buffer with size.
This function returns the same results as lua_load.
The name is the chunk name, used for debug information and error message.
The mode works as same as in function lua_load.
---
## luaL_loadfilex(L, filename, mode) -> [pop 0, push 1 result, raise out-of-memory error]
This function uses lua_load to load the chunk in the file named filename.
If filename is NULL, then it loads from standard input.
The first line in the file is ignored if it starts with a #.
The mode works as same as in function lua_load.
This function returns the same results as lua_load, but it has an extra error code LUA_ERRFILE.
*/

L_EXTERN l_funcindex /* sucess - a function pushed on the top, fail - stack remain unchanged */
l_luastate_load(lua_State* L, l_strn code)
{
  l_funcindex func = {0};

  if (!code.start || code.len <= 0 || code.len > L_MAX_RWSIZE) {
    return func;
  }

  if (luaL_loadbuffer(L, (const char*)code.start, (size_t)code.len, 0) != LUA_OK) {
    l_luastate_popError(L);
    return func;
  }

  func.index = lua_gettop(L);
  return func;
}

L_EXTERN l_funcindex /* sucess - a function pushed on the top, fail - stack remain unchanged */
l_luastate_loadfile(lua_State* L, l_strn filename)
{
  l_funcindex func = {0};
  if (!filename.start || filename.len <= 0) return func;

  if (luaL_loadfile(L, (const char*)filename.start) != LUA_OK) {
    l_luastate_popError(L);
    return func;
  }

  func.index = lua_gettop(L);
  return func;
}

/** call function
void lua_call(lua_State* L, int nargs, int nresults); [-(nargs+1), +nresults, e]
int lua_pcall(lua_State* L, int nargs, int nresults, int msgh); [-(nargs+1), +(nresults|1), -]
---
## lua_call(L, nargs, nresults) -> [pop (nargs+1) elements, push nresults result, may raise 1 error]
To call a function you must use the following protocol:
(1) first, the function to be called is pushed onto the stack;
(2) then, the arguments to the function are pushed in direct order, the first argument is pushed first;
(3) finally you call lua_call, nargs is the number of arguments that you pushed onto the stack.
All arguments and the function value are popped from the stack when the function is called.
The function results are pushed onto the stack when the function returns.
The number of results is adjusted to nresults, unless nresults is LUA_MULTRET (simply push all results).
Lua takes care that the returned values fit into the sapce, but it does not ensure any extra space in the stack.
The function results are pushed onto the stack in direct order (the first result is pushed first).
Any error inside the called function is propagated upwards (with a longjmp).
---
## lua_pcall(L, nargs, nresults, msgh) -> [pop (nargs+1) elements, push nresults or 1 error object, raise 0 error]
Call a function in protected mode. Both nargs and nresults have the same meaning as in lua_call.
If there are no errors during the call, lua_pcall behaves exactly like lua_call.
However, if there is any error, lua_pcall catches it, pushes an error object on the stack and returns an error code.
Like lua_call, lua_pcall always removes the function and its arguments from the stack first before push results.
If msgh is 0, then the error object returned on the stack is exactly the original error object.
Otherwise, msgh is the stack index of a message handler (this index cannot be a pseudo-index).
In case of runtime errors, this function will be called with the error object and its return value will be the object push on stack.
Typically, the message handler is used to add more debug information to the error object, such as a stack traceback.
Such information cannot be gathered after the return of lua_pcall, since by then the stack has unwound.
The lua_pcall function returns one of the following constants: LUA_OK(0), LUA_ERRRUN, LUA_ERRMEM, LUA_ERRERR, LUA_ERRGCMM.
*/

L_EXTERN int /* nargs are on top of the stack [-(nargs+1), +(nresults|0), -] */
l_luastate_call(lua_State* L, l_funcindex func, int nresults)
{
  if (func.index <= 0) {
    return false;
  }

  if (lua_pcall(L, lua_gettop(L) - func.index, nresults, /* msgh */ 0) != LUA_OK) {
    l_luastate_popError(L);
    return false;
  }

  return true;
}

L_EXTERN int /* load and call the function with no arguments */
l_luastate_exec(lua_State* L, l_strn code, int nresults)
{
  l_funcindex func = l_luastate_load(L, code);
  return l_luastate_call(L, func, nresults);
}

L_EXTERN int /* load and call the function with no arguments */
l_luastate_execfile(lua_State* L, l_strn filename, int nresults)
{
  l_funcindex func = l_luastate_loadfile(L, filename);
  return l_luastate_call(L, func, nresults);
}

/** lua globals
void lua_pop(lua_State* L, int n);   [-n, +0, -]
int lua_getglobal(lua_State* L, const char* name);   [-0, +1, e]
void lua_setglobal(lua_State* L, const char* name);   [-1, +0, e]
const char* lua_getupvalue(lua_State* L, int funcindex, int n);   [-0, +(0|1), -]
const char* lua_setupvalue(lua_State* L, int funcindex, int n);   [-(0|1), +0, -]
void lua_setfield(lua_State* L, int index, const char* k);   [-1, +0, e]
---
## basic functions
lua_pop(L, n) pops n elements from the stack.
---
## lua_getglobal(L, name)   [-0, +1, e]
Pushes onto the stack the value of the global name (may be nil).
Returns the type of that value.
## lua_setglobal(L, name)   [-1, +0, e]
Pops a value from the stack and sets it as the new value of global name.
---
## lua_getupvalue(L, funcindex, n)
Gets information about the n-th upvalue of the closure at index funcindex.
It pushes the upvalue's value onto the stack and returns its name.
Return NULL (and pushes nothing) when the index n is greater than the number of upvalues.
For C functions, this function uses the empty string "" as a name for all upvalues.
For Lua functions, upvalues are the external local variables that the function uses,
and that are consequently included in its closure.
---
## lua_setupvalue(L, funcindex, n)
Sets the value of a closure's upvalue.
It assigns the value at the top of the stack to the upvalue and returns its name.
It also pops up the value from the stack.
It returns NULL (and pops nothing) when the index n is greater than the number of upvalues.
---
## lua_setfield(L, tableindex, keyname)  [-1, +0, e]
Does the equivalent to t[k] = v, where t is the value at the given index and v is the value at the top of the stack.
This function pops the value from the stack.
As in Lua, this function may trigger a metamethod for the "newindex" event.
*/

#define L_LUACONF_TABLE_NAME "L_LUACONF_GLOBAL_TABLE"
#define L_LUACONF_MAX_FIELDNAME_LEN 80

L_EXTERN void /* the env table is on the top of the stack [-1, +0, -] */
l_luastate_setenv(lua_State* L, l_funcindex func)
{
  if (!lua_setupvalue(L, func.index, /* upvalue inex - _EVN is the 1st one */ 1)) {
    l_loge_s("luastate setenv failed");
    lua_pop(L, 1); /* returns NULL and pops nothing if lua_setupvalue() failed */
  }
}

L_EXTERN void /* the value is on the top of the stack [-1, +0, e] */
l_luastate_setfield(lua_State* L, l_tableindex table, const void* keyname)
{
  lua_setfield(L, table.index, (const char*)keyname); /* will pop the top value or raise error */
}

L_EXTERN l_tableindex
l_luastate_newtable(lua_State* L)
{
  /**
   * void lua_newtable(lua_State* L);   [-0, +1, m]
   * Creates a new empty table and pushes it onto the stack.
   * It is equivalent to lua_createtable(L, 0, 0).
   * ---
   * void lua_createtable(lua_State* L, int narr, int nrec);   [-0, +1, m]
   * Creates a new empty table and pushes it onto the stack.
   * Parameter narr is a hint for how many elements the table will have as a sequence;
   * parameter nrec is a hint for how many other elements the table will have.
   * Lua may use these hints to preallocate memory for the new table.
   * This preallocation is useful for performance when you know in advance how many elements the table will have.
   * Otherwise you can use the function lua_newtable.
   *
   */

  lua_newtable(L);
  return (l_tableindex){lua_gettop(L)};
}

static void
l_luaconf_init(lua_State* L)
{
  l_tableindex table;
  l_funcindex func;

  table = l_luastate_newtable(L); /* push table */
  lua_pushliteral(L, L_ROOT_DIR); /* push value */
  l_luastate_setfield(L, table, "rootdir"); /* pop value */
  lua_setglobal(L, L_LUACONF_TABLE_NAME); /* pop table */

  if (!l_luastate_execfile(L, l_strn_literal(L_ROOT_DIR "conf/init.lua"), 0)) {
    l_loge_s("execute init.lua failed");
    return;
  }

  func = l_luastate_loadfile(L, l_strn_literal(L_ROOT_DIR "conf/conf.lua")); /* push func if success */
  if (func.index == 0) {
    l_loge_s("load conf.lua failed");
    return;
  }

  lua_getglobal(L, L_LUACONF_TABLE_NAME); /* push table */
  l_luastate_setenv(L, func); /* pop table */

  l_luastate_call(L, func, 0); /* pop func */

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

static int
l_luaconf_get(lua_State* L, const l_byte* name)
{
  l_byte keyname[L_LUACONF_MAX_FIELDNAME_LEN+1] = {0};
  l_byte* keyend = 0;
  l_int len = 0;

  if (!name || name[0] == 0) {
    return false;
  }

  lua_getglobal(L, L_LUACONF_TABLE_NAME); /* push the table */

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
      if (len == L_LUACONF_MAX_FIELDNAME_LEN) {
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
l_luaconf_int(lua_State* L, const void* name)
{
  int startelems = 0;
  l_int result = 0;

  startelems = lua_gettop(L);

  if (!l_luaconf_get(L, (const l_byte*)name)) {
    lua_pop(L, lua_gettop(L) - startelems);
    l_logw_s("load int conf failed");
    return 0;
  }

  result = lua_tointeger(L, -1);
  lua_pop(L, lua_gettop(L) - startelems);
  return result;
}

L_EXTERN int
l_luaconf_str(lua_State* L, int (*func)(void* stream, l_strn str), void* stream, const void* name)
{
  int startelems = 0;
  const char* result = 0;
  size_t len = 0;

  startelems = lua_gettop(L);

  if (!l_luaconf_get(L, (const l_byte*)name)) {
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

  return func(stream, l_strn_n(result, len));
}

static int
l_luaconf_getv(lua_State* L, int n, va_list vl)
{
  const char* keyname = 0;
  if (n <= 0) return false;
  lua_getglobal(L, L_LUACONF_TABLE_NAME); /* push the table */
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
l_luaconf_intv(lua_State* L, int n, ...)
{
  int startelems = 0;
  l_int result = 0;
  va_list vl;
  va_start(vl, n);
  startelems = lua_gettop(L);

  if (!l_luaconf_getv(L, n, vl)) {
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
l_luaconf_strv(lua_State* L, int (*func)(void* stream, l_strn str), void* stream, int n, ...)
{
  int startelems = 0;
  const char* result = 0;
  size_t len = 0;
  va_list vl;
  va_start(vl, n);
  startelems = lua_gettop(L);

  if (!l_luaconf_getv(L, n, vl)) {
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

  return func(stream, l_strn_n(result, len));
}

/**
 * ## Continuations
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
void* lua_touserdata(lua_State* L, int index); // get userdata or NULL
*/


