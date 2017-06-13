#define l_auto_lib_h
#include "l_prefix.h"

/** basic types **
int/double use as usual
l_byte　l_sbyte l_rune true false
l_short l_medit l_integer l_large
l_ushort l_umedit l_uinteger l_ularge
l_float // speed float type */

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
  ccwriteline(file, "#ifndef CCLIB_AUTOCONF_H_%s#define CCLIB_AUTOCONF_H_", CCNEWLINE);
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

  ccwriteline(file, "%s/* false true nauty_bool nauty_char nauty_byte uoctet_int soctet_int */", CCNEWLINE);
  ccwriteline(file, "#undef false");
  ccwriteline(file, "#undef true");
  ccwriteline(file, "#undef nauty_bool");
  ccwriteline(file, "#undef nauty_char");
  ccwriteline(file, "#undef nauty_byte");
  ccwriteline(file, "#undef uoctet_int");
  ccwriteline(file, "#undef soctet_int");
  ccwriteline(file, "#undef ccoctet_int");
  ccwriteline(file, "#undef ccoctet_uint");
  ccwriteline(file, "#undef ccnauty_byte");
  ccwriteline(file, "#undef ccnauty_char");
  ccwriteline(file, "#define false 0");
  ccwriteline(file, "#define true 1");
  if (sizeof(unsigned char) == 1 && sizeof(signed char) == 1) {
    ccwriteline(file, "#define nauty_bool unsigned char");
    ccwriteline(file, "#define nauty_char unsigned char");
    ccwriteline(file, "#define nauty_byte unsigned char");
    ccwriteline(file, "#define uoctet_int unsigned char");
    ccwriteline(file, "#define soctet_int signed char");
    ccwriteline(file, "#define ccoctet_int signed char");
    ccwriteline(file, "#define ccoctet_uint unsigned char");
    ccwriteline(file, "#define ccnauty_byte unsigned char");
    ccwriteline(file, "#define ccnauty_char unsigned char");
  } else {
    ccloge("the size of char shall be 1-byte");
    ccwriteline(file, "#error \"the size of char shall be 1-byte\"");
  }

  ccwriteline(file, "%s/* sshort_int ushort_int - 16-bit */", CCNEWLINE);
  ccwriteline(file, "#undef sshort_int");
  ccwriteline(file, "#undef ushort_int");
  ccwriteline(file, "#undef ccshort_int");
  ccwriteline(file, "#undef ccshort_uint");
  if (sizeof(unsigned short) == 2 && sizeof(short) == 2) {
    ccwriteline(file, "#define sshort_int short");
    ccwriteline(file, "#define ushort_int unsigned short");
    ccwriteline(file, "#define ccshort_int short");
    ccwriteline(file, "#define ccshort_uint unsigned short");
  } else if (sizeof(unsigned int) == 2 && sizeof(int) == 2) {
    ccwriteline(file, "#define sshort_int int");
    ccwriteline(file, "#define ushort_int unsigned int");
    ccwriteline(file, "#define ccshort_int int");
    ccwriteline(file, "#define ccshort_uint unsigned int");
  } else {
    ccloge("no 16-bit integer type found");
    ccwriteline(file, "#error \"no 16-bit integer type found\"");
  }

  ccwriteline(file, "%s/* smedit_int umedit_int - 32-bit */", CCNEWLINE);
  ccwriteline(file, "#undef smedit_int");
  ccwriteline(file, "#undef umedit_int");
  ccwriteline(file, "#undef ccmedit_int");
  ccwriteline(file, "#undef ccmedit_uint");
  if (sizeof(unsigned int) == 4 && sizeof(int) == 4) {
    ccwriteline(file, "#define smedit_int int");
    ccwriteline(file, "#define umedit_int unsigned int");
    ccwriteline(file, "#define ccmedit_int int");
    ccwriteline(file, "#define ccmedit_uint unsigned int");
  } else if (sizeof(unsigned long) == 4 && sizeof(long) == 4) {
    ccwriteline(file, "#define smedit_int long");
    ccwriteline(file, "#define umedit_int unsigned long");
    ccwriteline(file, "#define ccmedit_int long");
    ccwriteline(file, "#define ccmedit_uint unsigned long");
  } else {
    ccloge("no 32-bit integer type found");
    ccwriteline(file, "#error \"no 32-bit integer type found\"");
  }

  ccwriteline(file, "%s/* sright_int uright_int - 64-bit */", CCNEWLINE);
  ccwriteline(file, "#undef sright_int");
  ccwriteline(file, "#undef uright_int");
  ccwriteline(file, "#undef ccnauty_int");
  ccwriteline(file, "#undef ccnauty_uint");
  if (sizeof(unsigned long) == 8 && sizeof(long) == 8) {
    ccwriteline(file, "#define sright_int long");
    ccwriteline(file, "#define ccnauty_int long");
    ccwriteline(file, "#define ccnauty_uint unsigned long");
    ccwriteline(file, "#define uright_int unsigned long");
  } else if (sizeof(unsigned long long) == 8 && sizeof(long long) == 8) {
    ccwriteline(file, "#define sright_int long long");
    ccwriteline(file, "#define ccnauty_int long long");
    ccwriteline(file, "#define ccnauty_uint unsigned long long");
    ccwriteline(file, "#define uright_int unsigned long long");
  } else {
    ccloge("no 64-bit integer type found");
    ccwriteline(file, "#error \"no 64-bit integer type found\"");
  }

  ccwriteline(file, "%s/* signed_ptr unsign_ptr - pointer-size integer */", CCNEWLINE);
  ccwriteline(file, "#undef signed_ptr");
  ccwriteline(file, "#undef unsign_ptr");
  ccwriteline(file, "#undef ccnauty_iptr");
  ccwriteline(file, "#undef ccnauty_uptr");
  if (sizeof(short) == sizeof(void*)) {
    ccwriteline(file, "#define signed_ptr short");
    ccwriteline(file, "#define unsign_ptr unsigned short");
    ccwriteline(file, "#define ccnauty_iptr");
    ccwriteline(file, "#define ccnauty_uptr");
  } else if (sizeof(int) == sizeof(void*)) {
    ccwriteline(file, "#define signed_ptr int");
    ccwriteline(file, "#define unsign_ptr unsigned int");
    ccwriteline(file, "#define ccnauty_iptr int");
    ccwriteline(file, "#define ccnauty_uptr unsigned int");
  } else if (sizeof(long) == sizeof(void*)) {
    ccwriteline(file, "#define signed_ptr long");
    ccwriteline(file, "#define unsign_ptr unsigned long");
    ccwriteline(file, "#define ccnauty_iptr long");
    ccwriteline(file, "#define ccnauty_uptr unsigned long");
  } else if (sizeof(long long) == sizeof(void*)) {
    ccwriteline(file, "#define signed_ptr long long");
    ccwriteline(file, "#define unsign_ptr unsigned long long");
    ccwriteline(file, "#define ccnauty_iptr long long");
    ccwriteline(file, "#define ccnauty_uptr unsigned long long");
  } else {
    ccloge("no pointer-size integer type found");
    ccwriteline(file, "#error \"no pointer-size integer type found\"");
  }

  ccwriteline(file, "%s/* handle_int */", CCNEWLINE);
  ccwriteline(file, "#undef handle_int");
  ccwriteline(file, "#undef cchandle_int");
  ccwriteline(file, "#define cchandle_int handle_int");
  if (LLIONFHDL_TYPE_BYTES == sizeof(short)) {
    ccwriteline(file, "#define handle_int %s", LLIONFHDL_TYPE_IS_SIGNED ? "short" : "unsigned short");
  } else if (LLIONFHDL_TYPE_BYTES == sizeof(int)) {
    ccwriteline(file, "#define handle_int %s", LLIONFHDL_TYPE_IS_SIGNED ? "int" : "unsigned int");
  } else if (LLIONFHDL_TYPE_BYTES == sizeof(long)) {
    ccwriteline(file, "#define handle_int %s", LLIONFHDL_TYPE_IS_SIGNED ? "long" : "unsigned long");
  } else if (LLIONFHDL_TYPE_BYTES == sizeof(long long)) {
    ccwriteline(file, "#define handle_int %s", LLIONFHDL_TYPE_IS_SIGNED ? "long long" : "unsigned long long");
  } else {
    ccloge("no handle-size integer type found");
    ccwriteline(file, "#error \"no handle-size integer type found\"");
  }

  ccwriteline(file, "%s/* slarge_int ularge_int - 128-bit */", CCNEWLINE);
  ccwriteline(file, "%s/* cclarge_int cclarge_uint - 128-bit */", CCNEWLINE);

  ccwriteline(file, "%s/* float point */", CCNEWLINE);
  ccwriteline(file, "#undef speed_real");
  ccwriteline(file, "#undef short_real");
  ccwriteline(file, "#undef right_real");
  ccwriteline(file, "#undef large_real");
  ccwriteline(file, "#define speed_real float");
  ccwriteline(file, "#define short_real float");
  ccwriteline(file, "#define right_real double");
  ccwriteline(file, "#define large_real long double");

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
  ccwriteline(file, "#define CC_IONFMGR_BYTES %d", LLIONFMGR_TYPE_BYTES);
  ccwriteline(file, "#define CC_SOCKADDR_BYTES %d", LLSOCKADDR_TYPE_BYTES);

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

  ccwriteline(file, "%s#endif /* CCLIB_AUTOCONF_H_ */", CCNEWLINE);
  fclose(file);
  return 0;
}