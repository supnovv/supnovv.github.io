#include "thatcore.h"
#include "luacapi.h"
int mainstart() {
  ccsetloglevel(4);
  ccthattest();
  ccluatest();
  ccplattest();
  return 0;
}

