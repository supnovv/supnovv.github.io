#define L_CORE_AUTO_CONFIG
#include "l_prefix.h"
#include "plationf.h"
#include "platsock.h"

#if defined(L_PLAT_WINDOWS)
#include "winpref.h"
#else
#include "linuxpref.h"
int l_mutex_size = sizeof(pthread_mutex_t);
int l_rwlock_size = sizeof(pthread_rwlock_t);
int l_condv_size = sizeof(pthread_cond_t);
int l_thrkey_size = sizeof(pthread_key_t);
int l_thrid_size = sizeof(pthread_t);
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <stddef.h>
#include <float.h>

#define l_byte unsigned char

static void l_log_e(const void* fmt, ...) {
  const l_byte* p = (const l_byte*)fmt;
  va_list args;
  for (; *p; ++p) {
    if (*p != '%') continue;
    if (*(p+1) == '%' || *(p+1) == 's') continue;
    l_log_e("format invalid: %s", fmt);
    return;
  }
  fprintf(stdout, "[E] ");
  va_start(args, fmt);
  vfprintf(stdout, (const char*)fmt, args);
  va_end(args);
  fprintf(stdout, L_NEWLINE);
}

static int l_write_line(FILE* self, const void* fmt, ...) {
  va_list args;
  int sz = 0;
  if (!self || !fmt) return 0;
  va_start(args, fmt);
  sz = vfprintf(self, (const char*)fmt, args);
  va_end(args);
  if (sz > 0) fprintf(self, L_NEWLINE);
  else l_log_e("vfprintf %s", strerror(errno));
  return sz;
}

typedef union {
  unsigned char c[sizeof(unsigned long)];
  unsigned long i;
} l_byteorder;

int main(void) {
  l_byteorder data;
  FILE* file = fopen("autoconf.h", "wb");
  if (!file) { l_log_e("fopen autoconf.h %s", strerror(errno)); return 1; }
  l_write_line(file, "#ifndef l_autoconf_lib_h%s#define l_autoconf_lib_h", L_NEWLINE);
  l_write_line(file, "#define _CRT_SECURE_NO_WARNINGS%s", L_NEWLINE);

  l_write_line(file, "/* platform bits */");
  l_write_line(file, "#undef L_PLAT_32BIT");
  l_write_line(file, "#undef L_PLAT_64BIT");
  if (sizeof(void*) == 4) {
    l_write_line(file, "#define L_PLAT_32BIT");
  } else if (sizeof(void*) == 8) {
    l_write_line(file, "#define L_PLAT_64BIT");
  } else {
    l_write_line(file, "/* unsupported %d-bit platform */", sizeof(void*)*8);
  }

  l_write_line(file, "%s/* byteorder */", L_NEWLINE);
  if (sizeof(unsigned long) < 4) {
    l_log_e("the size of long is less than 4-byte");
    l_write_line(file, "#error \"the size of long is less than 4-byte\"");
  }
  l_write_line(file, "#undef L_LIT_ENDIAN /* lower byte is stored at lower address */");
  l_write_line(file, "#undef L_BIG_ENDIAN /* lower byte is stored at higher address */");
  data.i = 0xabcdef;
  if (data.c[0] == 0xef) {
    l_write_line(file, "#define L_LIT_ENDIAN");
    if (data.c[1] != 0xcd || data.c[2] != 0xab || data.c[3] != 0x00) {
      l_log_e("little endian test failed");
      l_write_line(file, "#error \"little endian test failed\"");
    }
  }
  else {
    l_write_line(file, "#define L_BIG_ENDIAN");
    if (data.c[0] != 0x00 || data.c[1] != 0xab || data.c[2] != 0xcd || data.c[3] != 0xef) {
      l_log_e("big endian test failed");
      l_write_line(file, "#error \"big endian test failed\"");
    }
  }

  l_write_line(file, "%s/* false true l_rune l_byte l_sbyte */", L_NEWLINE);
  l_write_line(file, "#undef false");
  l_write_line(file, "#undef true");
  l_write_line(file, "#undef l_rune");
  l_write_line(file, "#undef l_byte");
  l_write_line(file, "#undef l_sbyte");
  l_write_line(file, "#define false 0");
  l_write_line(file, "#define true 1");
  if (sizeof(unsigned char) == 1 && sizeof(signed char) == 1) {
    l_write_line(file, "#define l_rune unsigned char");
    l_write_line(file, "#define l_byte unsigned char");
    l_write_line(file, "#define l_sbyte signed char");
  } else {
    l_log_e("the size of char shall be 1-byte");
    l_write_line(file, "#error \"the size of char shall be 1-byte\"");
  }

  l_write_line(file, "%s/* l_short l_ushort - 16-bit */", L_NEWLINE);
  l_write_line(file, "#undef l_short");
  l_write_line(file, "#undef l_ushort");
  if (sizeof(unsigned short) == 2 && sizeof(short) == 2) {
    l_write_line(file, "#define l_short short");
    l_write_line(file, "#define l_ushort unsigned short");
  } else if (sizeof(unsigned int) == 2 && sizeof(int) == 2) {
    l_write_line(file, "#define l_short int");
    l_write_line(file, "#define l_ushort unsigned int");
  } else {
    l_log_e("no 16-bit integer type found");
    l_write_line(file, "#error \"no 16-bit integer type found\"");
  }

  l_write_line(file, "%s/* l_medit l_umedit - 32-bit */", L_NEWLINE);
  l_write_line(file, "#undef l_medit");
  l_write_line(file, "#undef l_umedit");
  if (sizeof(unsigned int) == 4 && sizeof(int) == 4) {
    l_write_line(file, "#define l_medit int");
    l_write_line(file, "#define l_umedit unsigned int");
  } else if (sizeof(unsigned long) == 4 && sizeof(long) == 4) {
    l_write_line(file, "#define l_medit long");
    l_write_line(file, "#define l_umedit unsigned long");
  } else {
    l_log_e("no 32-bit integer type found");
    l_write_line(file, "#error \"no 32-bit integer type found\"");
  }

  l_write_line(file, "%s/* l_long l_ulong - 64-bit */", L_NEWLINE);
  l_write_line(file, "#undef l_long");
  l_write_line(file, "#undef l_ulong");
  if (sizeof(unsigned int) == 8 && sizeof(int) == 8) {
    l_write_line(file, "#define l_long int");
    l_write_line(file, "#define l_ulong unsigned int");
  } else if (sizeof(unsigned long) == 8 && sizeof(long) == 8) {
    l_write_line(file, "#define l_long long");
    l_write_line(file, "#define l_ulong unsigned long");
  } else if (sizeof(unsigned long long) == 8 && sizeof(long long) == 8) {
    l_write_line(file, "#define l_long long long");
    l_write_line(file, "#define l_ulong unsigned long long");
  } else {
    l_log_e("no 64-bit integer type found");
    l_write_line(file, "#error \"no 64-bit integer type found\"");
  }

  l_write_line(file, "%s/* l_int l_uint - pointer-size integer */", L_NEWLINE);
  l_write_line(file, "#undef l_int");
  l_write_line(file, "#undef l_uint");
  if (sizeof(short) == sizeof(void*)) {
    l_write_line(file, "#define l_int short");
    l_write_line(file, "#define l_uint unsigned short");
  } else if (sizeof(int) == sizeof(void*)) {
    l_write_line(file, "#define l_int int");
    l_write_line(file, "#define l_uint unsigned int");
  } else if (sizeof(long) == sizeof(void*)) {
    l_write_line(file, "#define l_int long");
    l_write_line(file, "#define l_uint unsigned long");
  } else if (sizeof(long long) == sizeof(void*)) {
    l_write_line(file, "#define l_int long long");
    l_write_line(file, "#define l_uint unsigned long long");
  } else {
    l_log_e("no pointer-size integer type found");
    l_write_line(file, "#error \"no pointer-size integer type found\"");
  }

  l_write_line(file, "%s/* l_handle */", L_NEWLINE);
  l_write_line(file, "#undef l_handle");
  if (L_HANDLE_TYPE_SIZE == sizeof(short)) {
    l_write_line(file, "#define l_handle %s", L_HANDLE_TYPE_IS_SIGNED ? "short" : "unsigned short");
  } else if (L_HANDLE_TYPE_SIZE == sizeof(int)) {
    l_write_line(file, "#define l_handle %s", L_HANDLE_TYPE_IS_SIGNED ? "int" : "unsigned int");
  } else if (L_HANDLE_TYPE_SIZE == sizeof(long)) {
    l_write_line(file, "#define l_handle %s", L_HANDLE_TYPE_IS_SIGNED ? "long" : "unsigned long");
  } else if (L_HANDLE_TYPE_SIZE == sizeof(long long)) {
    l_write_line(file, "#define l_handle %s", L_HANDLE_TYPE_IS_SIGNED ? "long long" : "unsigned long long");
  } else {
    l_log_e("no handle-size integer type found");
    l_write_line(file, "#error \"no handle-size integer type found\"");
  }

  l_write_line(file, "%s/* float point */", L_NEWLINE);
  l_write_line(file, "#undef l_float");
  l_write_line(file, "#define l_float float");

  l_write_line(file, "%s/* platform specific */", L_NEWLINE);
  l_write_line(file, "#undef L_MUTEX_SIZE");
  l_write_line(file, "#undef L_RWLOCK_SIZE");
  l_write_line(file, "#undef L_CONDV_SIZE");
  l_write_line(file, "#undef L_THKEY_SIZE");
  l_write_line(file, "#undef L_THRID_SIZE");
  l_write_line(file, "#undef L_IONFMGR_SIZE");
  l_write_line(file, "#undef L_SOCKADDR_SIZE");
  l_write_line(file, "#define L_MUTEX_SIZE %d", l_mutex_size);
  l_write_line(file, "#define L_RWLOCK_SIZE %d", l_rwlock_size);
  l_write_line(file, "#define L_CONDV_SIZE %d", l_condv_size);
  l_write_line(file, "#define L_THKEY_SIZE %d", l_thrkey_size);
  l_write_line(file, "#define L_THRID_SIZE %d", l_thrid_size);
  l_write_line(file, "#define L_IONFMGR_SIZE %d", L_IONFMGR_TYPE_SIZE);
  l_write_line(file, "#define L_SOCKADDR_SIZE %d", L_SOCKADDR_TYPE_SIZE);

  l_write_line(file, "%s/* char %d-bit */", L_NEWLINE, sizeof(char)*8);
  l_write_line(file, "/* short %d-bit */", sizeof(short)*8);
  l_write_line(file, "/* int %d-bit */", sizeof(int)*8);
  l_write_line(file, "/* long %d-bit */", sizeof(long)*8);
  l_write_line(file, "/* long long %d-bit */", sizeof(long long)*8);
  l_write_line(file, "/* void* %d-bit */", sizeof(void*)*8);
  l_write_line(file, "/* size_t %d-bit */", sizeof(size_t)*8);
  l_write_line(file, "/* time_t %d-bit */", sizeof(time_t)*8);
  l_write_line(file, "/* clock_t %d-bit */", sizeof(clock_t)*8);
  l_write_line(file, "/* ptrdiff_t %d-bit */", sizeof(ptrdiff_t)*8);
  l_write_line(file, "/* CLOCKS_PER_SEC %d */", CLOCKS_PER_SEC);
  l_write_line(file, "/* BUFSIZ %d */", BUFSIZ);
  l_write_line(file, "/* EOF %d */", EOF);
  l_write_line(file, "/* FILENAME_MAX %d */", FILENAME_MAX);
  l_write_line(file, "/* FOPEN_MAX %d */", FOPEN_MAX);
  l_write_line(file, "/* TMP_MAX %d */", TMP_MAX);
  l_write_line(file, "/* L_tmpnam %d */", L_tmpnam);
  l_write_line(file, "/* RAND_MAX %d */", RAND_MAX);
  l_write_line(file, "/* FLT_EPSILON %.80f */", FLT_EPSILON);
  l_write_line(file, "/* DBL_EPSILON %.80f */", DBL_EPSILON);

  l_write_line(file, "%s#endif /* l_autoconf_lib_h */", L_NEWLINE);
  fclose(file);
  return 0;
}
