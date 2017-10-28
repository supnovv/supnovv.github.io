#include "core/base.h"
#include "core/string.h"
#include "core/match.h"
#include "core/socket.h"
#include "core/service.h"

int l_test_start() {
  l_core_base_test();
  l_string_test();
  l_string_match_test();
  l_plat_core_test();
  l_plat_event_test();
  l_plat_sock_test();
  l_master_test();
  l_master_exit();
  return 0;
}

int main() {
  l_logger_setLevel(4);
  return startmainthread(l_test_start);
}

