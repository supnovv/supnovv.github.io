#include "thatcore.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"
#include "service.h"

static int start() {
  ccsetloglevel(4);
  ccthattest();
  ccplattest();
  ccluatest();
  ccplationftest();
  ccplatsocktest();
  return 0;
}

int main() {
  return startmainthread(start);
}

