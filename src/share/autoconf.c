#define _CRT_SECURE_NO_WARNINGS
#include "ccprefix.h"
#if defined(CC_OS_WINDOWS)
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#else
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <stddef.h>
#define byte unsigned char

#if defined(CC_OS_WINDOWS)
/* window platform */
#else
int ccmutexbytes = sizeof(pthread_mutex_t);
int ccrwlockbytes = sizeof(pthread_rwlock_t);
int cccondvbytes = sizeof(pthread_cond_t);
int ccthkeybytes = sizeof(pthread_key_t);
int ccthridbytes = sizeof(pthread_t);
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
  ccwriteline(file, "#define _CRT_SECURE_NO_WARNINGS%s", CCNEWLINE);

  ccwriteline(file, "/* platform bits */");
  ccwriteline(file, "#undef CC_PLAT_32BIT");
  ccwriteline(file, "#undef CC_PLAT_64BIT");
  if (sizeof(void*) == 4) {
    ccwriteline(file, "#define CC_PLAT_32BIT");
  } else if (sizeof(void*) == 8) {
    ccwriteline(file, "#define CC_PLAT_64BIT");
  } else {
    ccwriteline(file, "/* unsupported %d-bit platform */", sizeof(void*)*8);
  }

  ccwriteline(file, "%s/* byteorder */", CCNEWLINE);
  if (sizeof(unsigned long) < 4) {
    ccloge("the size of long is less than 4-byte");
    ccwriteline(file, "#error \"the size of long is less than 4-byte\"");
  }
  ccwriteline(file, "#undef CC_LIT_ENDIAN /* lower byte is stored at lower address */");
  ccwriteline(file, "#undef CC_BIG_ENDIAN /* lower byte is stored at higher address */");
  data.i = 0xabcdef;
  if (data.c[0] == 0xef) {
    ccwriteline(file, "#define CC_LIT_ENDIAN");
    if (data.c[1] != 0xcd || data.c[2] != 0xab || data.c[3] != 0x00) {
      ccloge("little endian test failed");
      ccwriteline(file, "#error \"little endian test failed\"");
    }
  }
  else {
    ccwriteline(file, "#define CC_BIG_ENDIAN");
    if (data.c[0] != 0x00 || data.c[1] != 0xab || data.c[2] != 0xcd || data.c[3] != 0xef) {
      ccloge("big endian test failed");
      ccwriteline(file, "#error \"big endian test failed\"");
    }
  }

  ccwriteline(file, "%s/* null false true bool byte int8 */", CCNEWLINE);
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
  } else {
    ccloge("the size of char shall be 1-byte");
    ccwriteline(file, "#error \"the size of char shall be 1-byte\"");
  }

  ccwriteline(file, "%s/* _short ushort - 16-bit */", CCNEWLINE);
  ccwriteline(file, "#undef _short");
  ccwriteline(file, "#undef ushort");
  if (sizeof(unsigned short) == 2 && sizeof(short) == 2) {
    ccwriteline(file, "#define _short short");
    ccwriteline(file, "#define ushort unsigned short");
  } else if (sizeof(unsigned int) == 2 && sizeof(int) == 2) {
    ccwriteline(file, "#define _short int");
    ccwriteline(file, "#define ushort unsigned int");
  } else {
    ccloge("no 16-bit integer type found");
    ccwriteline(file, "#error \"no 16-bit integer type found\"");
  }

  ccwriteline(file, "%s/* _medit umedit - 32-bit */", CCNEWLINE);
  ccwriteline(file, "#undef _medit");
  ccwriteline(file, "#undef umedit");
  if (sizeof(unsigned int) == 4 && sizeof(int) == 4) {
    ccwriteline(file, "#define _medit int");
    ccwriteline(file, "#define umedit unsigned int");
  } else if (sizeof(unsigned long) == 4 && sizeof(long) == 4) {
    ccwriteline(file, "#define _medit long");
    ccwriteline(file, "#define umedit unsigned long");
  } else {
    ccloge("no 32-bit integer type found");
    ccwriteline(file, "#error \"no 32-bit integer type found\"");
  }

  ccwriteline(file, "%s/* _int uint - 64-bit */", CCNEWLINE);
  ccwriteline(file, "#undef _int");
  ccwriteline(file, "#undef uint");
  if (sizeof(unsigned long) == 8 && sizeof(long) == 8) {
    ccwriteline(file, "#define _int long");
    ccwriteline(file, "#define uint unsigned long");
  } else if (sizeof(unsigned long long) == 8 && sizeof(long long) == 8) {
    ccwriteline(file, "#define _int long long");
    ccwriteline(file, "#define uint unsigned long long");
  } else {
    ccloge("no 64-bit integer type found");
    ccwriteline(file, "#error \"no 64-bit integer type found\"");
  }

  ccwriteline(file, "%s/* _intptr uintptr - pointer-size integer */", CCNEWLINE);
  ccwriteline(file, "#undef _intptr");
  ccwriteline(file, "#undef uintptr");
  if (sizeof(short) == sizeof(void*)) {
    ccwriteline(file, "#define _intptr short");
    ccwriteline(file, "#define uintptr unsigned short");
  } else if (sizeof(int) == sizeof(void*)) {
    ccwriteline(file, "#define _intptr int");
    ccwriteline(file, "#define uintptr unsigned int");
  } else if (sizeof(long) == sizeof(void*)) {
    ccwriteline(file, "#define _intptr long");
    ccwriteline(file, "#define uintptr unsigned long");
  } else if (sizeof(long long) == sizeof(void*)) {
    ccwriteline(file, "#define _intptr long long");
    ccwriteline(file, "#define uintptr unsigned long long");
  } else {
    ccloge("no pointer-size integer type found");
    ccwriteline(file, "#error \"no pointer-size integer type found\"");
  }

  ccwriteline(file, "%s/* _long ulong - 128-bit */", CCNEWLINE);

  ccwriteline(file, "%s/* float point */", CCNEWLINE);
  ccwriteline(file, "#undef freal");
  ccwriteline(file, "#define freal double");

  ccwriteline(file, "%s/* platform specific */", CCNEWLINE);
  ccwriteline(file, "#undef CC_MUTEX_BYTES");
  ccwriteline(file, "#undef CC_RWLOCK_BYTES");
  ccwriteline(file, "#undef CC_CONDV_BYTES");
  ccwriteline(file, "#undef CC_THKEY_BYTES");
  ccwriteline(file, "#undef CC_THRID_BYTES");
  ccwriteline(file, "#define CC_MUTEX_BYTES %d", ccmutexbytes);
  ccwriteline(file, "#define CC_RWLOCK_BYTES %d", ccrwlockbytes);
  ccwriteline(file, "#define CC_CONDV_BYTES %d", cccondvbytes);
  ccwriteline(file, "#define CC_THKEY_BYTES %d", ccthkeybytes);
  ccwriteline(file, "#define CC_THRID_BYTES %d", ccthridbytes);

  ccwriteline(file, "%s/* char %d-bit */", CCNEWLINE, sizeof(char)*8);
  ccwriteline(file, "/* short %d-bit */", sizeof(short)*8);
  ccwriteline(file, "/* int %d-bit */", sizeof(int)*8);
  ccwriteline(file, "/* long %d-bit */", sizeof(long)*8);
  ccwriteline(file, "/* long long %d-bit */", sizeof(long long)*8);
  ccwriteline(file, "/* void* %d-bit */", sizeof(void*)*8);
  ccwriteline(file, "/* size_t %d-bit */", sizeof(size_t)*8);
  ccwriteline(file, "/* time_t %d-bit */", sizeof(time_t)*8);
  ccwriteline(file, "/* clock_t %d-bit */", sizeof(clock_t)*8);
  ccwriteline(file, "/* ptrdiff_t %d-bit */", sizeof(ptrdiff_t)*8);
  ccwriteline(file, "/* CLOCKS_PER_SEC %d */", CLOCKS_PER_SEC);
  ccwriteline(file, "/* BUFSIZ %d */", BUFSIZ);
  ccwriteline(file, "/* EOF %d */", EOF);
  ccwriteline(file, "/* FILENAME_MAX %d */", FILENAME_MAX);
  ccwriteline(file, "/* FOPEN_MAX %d */", FOPEN_MAX);
  ccwriteline(file, "/* TMP_MAX %d */", TMP_MAX);
  ccwriteline(file, "/* L_tmpnam %d */", L_tmpnam);
  ccwriteline(file, "/* RAND_MAX %d */", RAND_MAX);

  ccwriteline(file, "%s#endif /* LIBC_AUTOCONF_H_ */", CCNEWLINE);
  fclose(file);
  return 0;
}
