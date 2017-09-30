#include "core/base.h"

int l_test_start() {
  l_core_base_test();
  l_luac_test();
  l_plat_test();
  l_plat_ionf_test();
  l_plat_sock_test();
  l_string_test();
  l_master_exit();
  return 0;
}

int main() {
  l_logger_setLevel(4);
  return startmainthread(l_test_start);
}

