#ifndef CCLIB_LUACAPI_H_
#define CCLIB_LUACAPI_H_
#include "thatcore.h"

struct lua_State;
CORE_API struct lua_State* cclua_newstate();
CORE_API void cclua_close(struct lua_State* L);

struct ccluaco {
  struct ccsmplnode node;
  struct ccthread* owner;
  struct lua_State* co;
  int coref;
  int (*func)(struct ccluaco*);
  int (*kfunc)(struct ccluaco*);
  void* work;
  void* msg;
};

CORE_API struct ccluaco ccluaco_create(struct ccthread* owner, int (*func)(struct ccluaco*), void* work);
CORE_API void ccluaco_free(struct ccluaco* co);
CORE_API int ccluaco_resume(struct ccluaco* co);
CORE_API int ccluaco_yield(struct ccluaco* co, int (*kfunc)(struct ccluaco*));
CORE_API void ccluatest();

#endif /* CCLIB_LUACAPI_H_ */

