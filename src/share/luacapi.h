#ifndef CCLIB_LUACAPI_H_
#define CCLIB_LUACAPI_H_
#include "thatcore.h"
#include "service.h"

struct lua_State;
CORE_API struct lua_State* cclua_newstate();
CORE_API void cclua_close(struct lua_State* L);

struct ccluaco {
  struct ccsmplnode node;
  struct lua_State* L;
  struct lua_State* co;
  int coref;
  int (*func)(struct ccluaco*);
  int (*kfunc)(struct ccluaco*);
  struct ccservice* srvc;
  struct ccmsgnode* msg;
};

CORE_API nauty_bool ccluaco_init(struct ccluaco* co, struct lua_State* L, int (*func)(struct ccluaco*), struct ccservice* srvc);
CORE_API void ccluaco_free(struct ccluaco* co);
CORE_API int ccluaco_resume(struct ccluaco* co);
CORE_API int ccluaco_yield(struct ccluaco* co, int (*kfunc)(struct ccluaco*));
CORE_API void ccluatest();

#endif /* CCLIB_LUACAPI_H_ */

