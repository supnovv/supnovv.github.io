#include "thatcore.h"
int core_startmainthread(int (*start)(), int argc, char** argv);
int main(int argc, char** argv) {
  return core_startmainthread(mainentry, argc, argv);
}
