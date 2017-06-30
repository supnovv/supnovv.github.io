#include "thatcore.h"
#include "l_socket.h"
#include "l_service.h"
#include "l_state.h"
#include "l_master.h"

int l_test_start() {
  l_core_test();
  l_luac_test();
  l_plat_test();
  l_plat_ionf_test();
  l_plat_sock_test();
  l_string_test();
  return 0;
}

int main() {
  l_set_log_level(4);
  return startmainthread(l_test_start);
}

