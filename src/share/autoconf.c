#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <stddef.h>
#include "ccprefix.h"
#define byte unsigned char

#if defined(CC_OS_WINDOWS)
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#else
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
int ccmutexbytes = sizeof(pthread_mutex_t);
int ccrwlockbytes = sizeof(pthread_rwlock_t);
int cccondvbytes = sizeof(pthread_cond_t);
int ccunitbytes = sizeof(struct cctypeunit);
#endif

static void ccloge(const void* fmt, ...) {
  const byte* p = (const byte*)fmt;
  va_list args;
  for (; *p; ++p) {
    if (*p != '%') continue;
    if (*(p+1) == '%' || *(p+1) == 's') continue;
    ccloge("format invalid: %s", fmt);
    return;
  }
  fprintf(stdout, "[E] ");
  va_start(args, fmt);
  vfprintf(stdout, (const char*)fmt, args);
  va_end(args);
  fprintf(stdout, CCNEWLINE);
}

static int ccwriteline(FILE* self, const void* fmt, ...) {
  va_list args;
  int sz = 0;
  if (!self || !fmt) return 0;
  va_start(args, fmt);
  sz = vfprintf(self, (const char*)fmt, args);
  va_end(args);
  if (sz > 0) fprintf(self, CCNEWLINE);
  else ccloge("vfprintf %s", strerror(errno));
  return sz;
}

union ccbyteorder {
  unsigned char c[sizeof(unsigned long)];
  unsigned long i;
};

int main(void) {
  union ccbyteorder data;
  FILE* file = fopen("autoconf.h", "wb");
  if (!file) { ccloge("fopen autoconf.h %s", strerror(errno)); return 1; }
  ccwriteline(file, "#ifndef LIBC_AUTOCONF_H_%s#define LIBC_AUTOCONF_H_", CCNEWLINE);
  ccwriteline(file, "#define _CRT_SECURE_NO_WARNINGS");
  ccwriteline(file, "/* platform and byteorder */");
  ccwriteline(file, "#undef CC32BITPLAT");
  ccwriteline(file, "#undef CC64BITPLAT");
  if (sizeof(void*) == 4) {
    ccwriteline(file, "#define CC32BITPLAT");
  } else if (sizeof(void*) == 8) {
    ccwriteline(file, "#define CC64BITPLAT");
  } else {
    ccwriteline(file, "/* unsupported %d-bit platform */", sizeof(void*)*8);
  }
  if (sizeof(unsigned long) < 4) {
    ccloge("the size of long is less than 4-byte");
    ccwriteline(file, "#error \"the size of long is less than 4-byte\"");
  }
  ccwriteline(file, "#undef CCLITENDIAN /* lower byte is stored at lower address */");
  ccwriteline(file, "#undef CCBIGENDIAN /* lower byte is stored at higher address */");
  data.i = 0xabcdef;
  if (data.c[0] == 0xef) {
    ccwriteline(file, "#define CCLITENDIAN");
    if (data.c[1] != 0xcd || data.c[2] != 0xab || data.c[3] != 0x00) {
      ccloge("little endian test failed");
      ccwriteline(file, "#error \"little endian test failed\"");
    }
  }
  else {
    ccwriteline(file, "#define CCBIGENDIAN");
    if (data.c[0] != 0x00 || data.c[1] != 0xab || data.c[2] != 0xcd || data.c[3] != 0xef) {
      ccloge("big endian test failed");
      ccwriteline(file, "#error \"big endian test failed\"");
    }
  }
  ccwriteline(file, "/* null false true bool byte int8 */");
  ccwriteline(file, "#undef null");
  ccwriteline(file, "#undef false");
  ccwriteline(file, "#undef true");
  ccwriteline(file, "#undef bool");
  ccwriteline(file, "#undef byte");
  ccwriteline(file, "#undef int8");
  ccwriteline(file, "#define null 0");
  ccwriteline(file, "#define false 0");
  ccwriteline(file, "#define true 1");
  if (sizeof(unsigned char) == 1 && sizeof(signed char) == 1) {
    ccwriteline(file, "#define bool unsigned char");
    ccwriteline(file, "#define byte unsigned char");
    ccwriteline(file, "#define int8 signed char");
  }
  else {
    ccloge("the size of char shall be 1-byte");
    ccwriteline(file, "#error \"the size of char shall be 1-byte\"");
  }
  ccwriteline(file, "/* sshort ushort - 16-bit */");
  ccwriteline(file, "#undef sshort");
  ccwriteline(file, "#undef ushort");
  if (sizeof(unsigned short) == 2 && sizeof(short) == 2) {
    ccwriteline(file, "#define sshort short");
    ccwriteline(file, "#define ushort unsigned short");
  }
  else if (sizeof(unsigned int) == 2 && sizeof(int) == 2) {
    ccwriteline(file, "#define sshort int");
    ccwriteline(file, "#define ushort unsigned int");
  }
  else {
    ccloge("no 16-bit integer type found");
    ccwriteline(file, "#error \"no 16-bit integer type found\"");
  }
  ccwriteline(file, "/* smedit umedit - 32-bit */");
  ccwriteline(file, "#undef smedit");
  ccwriteline(file, "#undef umedit");
  if (sizeof(unsigned int) == 4 && sizeof(int) == 4) {
    ccwriteline(file, "#define smedit int");
    ccwriteline(file, "#define umedit unsigned int");
  }
  else if (sizeof(unsigned long) == 4 && sizeof(long) == 4) {
    ccwriteline(file, "#define smedit long");
    ccwriteline(file, "#define umedit unsigned long");
  }
  else {
    ccloge("no 32-bit integer type found");
    ccwriteline(file, "#error \"no 32-bit integer type found\"");
  }
  ccwriteline(file, "/* sint uint - 64-bit */");
  ccwriteline(file, "#undef sint");
  ccwriteline(file, "#undef uint");
  if (sizeof(unsigned long) == 8 && sizeof(long) == 8) {
    ccwriteline(file, "#define sint long");
    ccwriteline(file, "#define uint unsigned long");
  }
  else if (sizeof(unsigned long long) == 8 && sizeof(long long) == 8) {
    ccwriteline(file, "#define sint long long");
    ccwriteline(file, "#define uint unsigned long long");
  }
  else {
    ccloge("no 64-bit integer type found");
    ccwriteline(file, "#error \"no 64-bit integer type found\"");
  }
  ccwriteline(file, "/* float point */");
  ccwriteline(file, "#undef freal");
  ccwriteline(file, "#define freal double");
  ccwriteline(file, "/* platform specific */");
  ccwriteline(file, "#undef CCMUTEX_NUNIT");
  ccwriteline(file, "#undef CCRWLOCK_NUNIT");
  ccwriteline(file, "#undef CCCONDV_NUNIT");
  ccwriteline(file, "#define CCMUTEX_NUNIT %d", (ccmutexbytes + ccunitbytes - 1) / ccunitbytes);
  ccwriteline(file, "#define CCRWLOCK_NUNIT %d", (ccrwlockbytes + ccunitbytes - 1) / ccunitbytes);
  ccwriteline(file, "#define CCCONDV_NUNIT %d", (cccondvbytes + ccunitbytes - 1) / ccunitbytes);
  ccwriteline(file, "/* void* %.0f-bit */", (double)sizeof(void*)*8);
  ccwriteline(file, "/* size_t %.0f-bit */", (double)sizeof(size_t)*8);
  ccwriteline(file, "/* ptrdiff_t %.0f-bit */", (double)sizeof(ptrdiff_t)*8);
  ccwriteline(file, "/* BUFSIZ %.0f */", (double)BUFSIZ);
  ccwriteline(file, "/* EOF %.0f */", (double)EOF);
  ccwriteline(file, "/* FILENAME_MAX %.0f */", (double)FILENAME_MAX);
  ccwriteline(file, "/* FOPEN_MAX %.0f */", (double)FOPEN_MAX);
  ccwriteline(file, "/* TMP_MAX %.0f */", (double)TMP_MAX);
  ccwriteline(file, "/* L_tmpnam %.0f */", (double)L_tmpnam);
  ccwriteline(file, "/* RAND_MAX %.0f */", (double)RAND_MAX);
  ccwriteline(file, "/* CLOCKS_PER_SEC %.0f */", (double)CLOCKS_PER_SEC);
  ccwriteline(file, "/* clock_t %.0f-bit */", (double)sizeof(clock_t)*8);
  ccwriteline(file, "/* time_t %.0f-bit */", (double)sizeof(time_t)*8);
  ccwriteline(file, "#endif /* LIBC_AUTOCONF_H_ */");
  fclose(file);
  return 0;
}
