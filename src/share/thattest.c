#include "thatcore.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"

int mainstart() {
  ccsetloglevel(4);
  ccthattest();
  ccplattest();
  ccluatest();
  ccplationftest();
  ccplatsocktest();
  return 0;
}

