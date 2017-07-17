#include "lucycore.h"

int l_test_start() {
  l_core_test();
  l_luac_test();
  l_plat_test();
  l_plat_ionf_test();
  l_plat_sock_test();
  l_string_test();
  l_master_exit();
  return 0;
}

int main() {
  l_set_log_level(4);
  return startmainthread(l_test_start);
}

