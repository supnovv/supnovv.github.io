#include <stdio.h>
#include "thatcore.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"
#include "service.h"
#include "string.h"

int cctest_start() {
  ccsetloglevel(4);
  ccthattest();
  ccluatest();
  ccplattest();
  ccplationftest();
  ccplatsocktest();
  ccstringtest();
  return 0;
}

int main() {
  return startmainthread(cctest_start);
}

