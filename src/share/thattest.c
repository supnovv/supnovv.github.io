#include "thatcore.h"
#include "luacapi.h"
#include "socket.h"
int mainstart() {
  ccsetloglevel(4);
  ccthattest();
  ccplattest();
  ccluatest();
  ccplatsocktest();
  return 0;
}

