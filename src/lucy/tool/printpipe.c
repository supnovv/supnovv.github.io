#include <stdio.h>

int main() {
  unsigned char ch = 0;
  while (fread(&ch, 1, 1, stdin) == 1) {
    fwrite(&ch, 1, 1, stdout);
  }
  return 0;
}

