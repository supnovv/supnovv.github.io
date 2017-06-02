#ifndef CCLIB_LUACAPI_H_
#define CCLIB_LUACAPI_H_
#include "thatcore.h"

struct lua_State;
CORE_API struct lua_State* cclua_newstate();
CORE_API void cclua_close(struct lua_State* L);

struct ccrobot;
struct ccmessage;
struct ccstate {
  struct ccsmplnode node;
  struct lua_State* L;
  struct lua_State* co;
  int coref;
  int (*func)(struct ccstate*);
  int (*kfunc)(struct ccstate*);
  struct ccrobot* bot;
  struct ccmessage* msg;
};

CORE_API nauty_bool ccstate_init(struct ccstate* co, struct lua_State* L, int (*func)(struct ccstate*), struct ccrobot* bot);
CORE_API void ccstate_free(struct ccstate* co);
CORE_API int ccstate_resume(struct ccstate* co);
CORE_API int ccstate_yield(struct ccstate* co, int (*kfunc)(struct ccstate*));
CORE_API void ccluatest();

#endif /* CCLIB_LUACAPI_H_ */

