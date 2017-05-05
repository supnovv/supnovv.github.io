#include "thatcore.h"
int llmainthreadfunc(int (*start)(), int argc, char** argv);
int main(int argc, char** argv) {
  int mainstart();
  return llmainthreadfunc(mainstart, argc, argv);
}

