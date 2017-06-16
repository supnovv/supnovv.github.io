#include "thatcore.h"
#include "luacapi.h"
#include "ionfmgr.h"
#include "socket.h"
#include "service.h"
#include "string.h"

int l_test_start() {
  l_set_log_level(4);
  l_core_test();
  l_luac_test();
  l_plat_test();
  l_plat_ionf_test();
  l_plat_sock_test();
  l_string_test();
  return 0;
}

int main() {
  return startmainthread(l_test_start);
}

