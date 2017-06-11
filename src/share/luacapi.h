#ifndef CCLIB_LUACAPI_H_
#define CCLIB_LUACAPI_H_
#include "thatcore.h"

typedef struct lua_State lua_State;
CORE_API struct lua_State* cclua_newstate();
CORE_API void cclua_close(lua_State* L);

typedef struct ccservice ccservice;
typedef struct ccmessage ccmessage;
typedef struct ccstate {
  struct ccsmplnode node;
  lua_State* L;
  lua_State* co;
  int coref;
  int (*func)(struct ccstate*);
  int (*kfunc)(struct ccstate*);
  ccservice* srvc;
  ccmessage* msg;
} ccstate;

CORE_API nauty_bool ccstate_init(ccstate* co, lua_State* L, int (*func)(ccstate*), ccservice* srvc);
CORE_API void ccstate_free(ccstate* co);
CORE_API int ccstate_resume(ccstate* co);
CORE_API int ccstate_yield(ccstate* co, int (*kfunc)(ccstate*));
CORE_API void ccluatest();

#endif /* CCLIB_LUACAPI_H_ */

