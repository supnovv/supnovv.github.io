#define L_LIBRARY_IMPL
#include "lucycore.h"

typedef struct {
  int argc;
  int curc;
  char** argv;
  l_byte* arg0;
  double farg;
  l_int iarg;
  l_strn sarg;
} l_cmdargs;

l_spec_extern(l_cmdargs)
l_cmdargs_init(int argc, char** argv) {
  l_cmdargs a = {0};
  if (argc > 1 && argv) {
    a.argc = argc - 1;
    a.argv = argv + 1;
  }
  a.arg0 = (l_byte*)argv[0];
  return a;
}

l_spec_extern(int)
l_cmdargs_readString(l_cmdargs* self, void (*func)(void* obj, l_strn str), void* obj) {
  
}

l_spec_extern(l_int)
l_cmdargs_readInt(l_cmdargs* self) {
  
}
