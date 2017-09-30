#include <stdio.h>

#define CMDLINE_BUFFER_SIZE 8*1024*2
unsigned char cmdline_buffer[CMDLINE_BUFFER_SIZE+1];

int main(int argc, char** argv) {
  if (argc > 1) {
    int i = 1;
    for (; i < argc; ++i) {
      printf("%s\n", argv[i]);
    }
  }
  else {
    size_t n = fread(cmdline_buffer, 1, CMDLINE_BUFFER_SIZE, stdin);
    if (n == CMDLINE_BUFFER_SIZE || feof(stdin)) {
      /* read success */
    } else {
      n = 0;
    }
    cmdline_buffer[n] = 0;
    
  }
  return 0;
}

