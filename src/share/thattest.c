#include "thatcore.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"
#include "service.h"

int cctest_start() {
  ccsetloglevel(4);
  ccthattest();
  ccplattest();
  ccluatest();
  ccplationftest();
  ccplatsocktest();
  return 0;
}

int main() {
  return startmainthread(cctest_start);
}

