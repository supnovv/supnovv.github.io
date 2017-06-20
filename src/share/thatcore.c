#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#define L_CORELIB_IMPL
#include "thatcore.h"

void l_zero_l(void* start, l_int len) {
  if (!start || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("invalid %d", ld(len));
    return;
  }
  /* void* memset(void* ptr, int value, size_t num); */
  memset(start, 0, (size_t)len);
}

void l_copy_l(const void* from, l_int len, void* to) {
  if (!from || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("invalid %d", ld(len));
    return;
  }
  if (l_rstr(to) + len <= l_rstr(from) || l_rstr(to) >= l_rstr(from) + len) {
    /* void* memcpy(void* destination, const void* source, size_t num);
    To avoid overflows, the size of the arrays pointed to by both the
    destination and source parameters, shall be at least num bytes,
    and should not overlap (for overlapping memory blocks, memmove is
    a safer approach). */
    memcpy(to, from, l_cast(size_t, len));
  } else {
    /* void* memmove(void* destination, const void* source, size_t num);
    Copying takes place as if an intermediate buffer were used,
    allowing the destination and source to overlap. */
    memmove(to, from, l_cast(size_t, len));
  }
}

static void* l_out_of_memory(l_int size, int init) {
  l_process_exit();
  (void)size;
  (void)init;
  return 0;
}

static l_int l_check_alloc_size(l_int size) {
  if (size <= 0 || size > l_max_rdwr_size) return 0;
  return (((size - 1) >> 3) + 1) << 3; /* times of eight */
}

void* l_raw_malloc(l_int size) {
  void* p = 0;
  l_int n = l_check_alloc_size(size);
  if (!n) { l_loge_1("large %d", ld(size)); return 0; }
  p = malloc(l_cast(size_t, n));
  if (p) return p; /* not init */
  return l_out_of_memory(n, 0);
}

void* l_raw_calloc(l_int size) {
  void* p = 0;
  l_int n = l_check_alloc_size(size);
  if (!n) { l_loge_1("large %d", ld(size)); return 0; }
  /* void* calloc(size_t num, size_t size); */
  p = calloc(l_cast(size_t, n) >> 3, 8);
  if (p) return p;
  return l_out_of_memory(n, 1);
}

void* l_raw_realloc(void* p, l_int old, l_int newsz) {
  void* temp = 0;
  l_int n = l_check_alloc_size(newsz);
  if (!p || old <= 0 || n == 0) { l_loge_1("invalid %d", ld(newsz)); return 0; }

  /** void* realloc(void* buffer, size_t size); **
  Changes the size of the memory block pointed by buffer. The function
  may move the memory block to a new location (its address is returned
  by the function). The content of the memory block is preserved up to
  the lesser of the new and old sizes, even if the block is moved to a
  new location. ***If the new size is larger, the value of the newly
  allocated portion is indeterminate***.
  In case of that buffer is a null pointer, the function behaves like malloc,
  assigning a new block of size bytes and returning a pointer to its beginning.
  If size is zero, the memory previously allocated at buffer is deallocated
  as if a call to free was made, and a null pointer is returned. For c99/c11,
  the return value depends on the particular library implementation, it may
  either be a null pointer or some other location that shall not be dereference.
  If the function fails to allocate the requested block of memory, a null
  pointer is returned, and the memory block pointed to by buffer is not
  deallocated (it is still valid, and with its contents unchanged). */

  if (n > old) {
    temp = realloc(p, l_cast(size_t, n));
    if (temp) { /* the newly allocated portion is indeterminate */
      l_zero_l(temp + old, n - old);
      return temp;
    }
    if ((temp = l_out_of_memory(n, 0))) {
      l_copy_l(p, old, temp);
      l_zero_l(temp + old, n - old);
      l_raw_free(p);
      return temp;
    }
  } else {
    temp = realloc(p, l_cast(size_t, n));
    if (temp) return temp;
    if ((temp = l_out_of_memory(n, 0))) {
      l_copy_l(p, n, temp);
      l_raw_free(p);
      return temp;
    }
  }
  return 0;
}

void l_raw_free(void* p) {
  if (p == 0) return;
  free(p);
}

static l_filestream llopenfile(const void* name, const char* mode) {
  l_filestream fs = {0};
  if (!name) { l_loge_s("empty name"); return fs; }
  fs.stream = fopen((const char*)name, mode);
  if (!fs.stream) {
    l_loge_1("fopen %s", lserror(errno));
  }
  return fs;
}

static void llreopenfile(FILE* file, const void* name, const char* mode) {
  if (!name) { l_loge_s("empty name"); return; }
  if (freopen((const char*)name, mode, file) == 0) {
    l_loge_1("freopen %s", lserror(errno));
  }
}

l_filestream l_open_read(const void* name) {
  return llopenfile(name, "rb");
}

l_filestream l_open_write(const void* name) {
  return llopenfile(name, "wb");
}

l_filestream l_open_append(const void* name) {
  return llopenfile(name, "ab");
}

l_filestream l_open_read_write(const void* name) {
  return llopenfile(name, "rb+");
}

l_filestream l_open_write_unbuffered(const void* name) {
  l_filestream fs = llopenfile(name, "wb");
  if (!fs.stream) return fs;
  setbuf((FILE*)fs.stream, 0);
  return fs;
}

l_extern l_filestream l_open_append_unbuffered(const void* name) {
  l_filestream fs = llopenfile(name, "ab");
  if (!fs.stream) return fs;
  setbuf((FILE*)fs.stream, 0);
  return fs;
}

int l_remove_file(const void* name) {
  if (!name) { l_loge_s("empty name"); return L_STATUS_EINVAL; }
  if (remove((const char*)name) != 0) {
    l_loge_1("remove %s", lserror(errno));
    return L_STATUS_ERROR;
  }
  return 0;
}

int l_rename_file(const void* from, const void* to) {
  /* int rename(const char* oldname, const char* newname);
  Changes the name of the file or directory specified by
  oldname to newname. This is an operation performed directly
  on a file; No streams are involved in the operation. If
  oldname and newname specify different paths and this is
  supported by the system, the file is moved to the new
  location. If newname names an existing file, the function
  may either fail or override the existing file, depending on
  the specific system and library implementation. */
  if (!from || !to) { l_loge_s("empty name"); return L_STATUS_EINVAL; }
  if (rename((const char*)from, (const char*)to) != 0) {
    l_loge_1("rename %s", lserror(errno));
    return L_STATUS_ERROR;
  }
  return 0;
}

void l_redirect_stdout(const void* name) {
  llreopenfile(stdout, name, "wb");
}

void l_redirect_stderr(const void* name) {
  llreopenfile(stderr, name, "wb");
}

void l_reditect_stdin(const void* name) {
  llreopenfile(stdin, name, "rb");
}

void l_close_file(l_filestream* self) {
  if (!self->stream) return;
  if (fclose((FILE*)self->stream) != 0) {
    l_loge_1("fclose %s", lserror(errno));
  }
  self->stream = 0;
}

void l_flush_file(l_filestream* self) {
  if (fflush((FILE*)self->stream) != 0) {
    l_loge_1("fflush %s", lserror(errno));
  }
}

void l_rewind_file(l_filestream* self) {
  rewind((FILE*)self->stream);
}

void l_seek_from_begin(l_filestream* self, long offset) {
  if (fseek((FILE*)self->stream, offset, SEEK_SET) != 0) {
    l_loge_1("fseek SET %s", lserror(errno));
  }
}

void l_seek_from_curpos(l_filestream* self, long offset) {
  if (fseek((FILE*)self->stream, offset, SEEK_CUR) != 0) {
    l_loge_1("fseek CUR %s", lserror(errno));
  }
}

void l_clear_file_error(l_filestream* self) {
  clearerr((FILE*)self->stream);
}

l_int l_write_file(l_filestream* self, const void* s, l_int len) {
  l_int n = 0;
  if (!s || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("invalid %d", ld(len));
    return 0;
  }
  if ((n = (l_int)fwrite(s, 1, (size_t)len, (FILE*)self->stream)) != len) {
    l_loge_1("fwrite %s", lserror(errno));
    if (n <= 0) return 0;
  }
  return n;
}

l_int l_read_file(l_filestream* self, void* out, l_int len) {
  l_int n = 0;
  if (!out || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("invalid %d", ld(len));
    return 0;
  }
  if ((n = (l_int)fread(out, 1, (size_t)len, (FILE*)self->stream)) != len) {
    if (!feof((FILE*)self->stream)) {
      l_loge_1("fread %s", lserror(errno));
    }
    if (n <= 0) return 0;
  }
  return n;
}

#define L_FORMAT_HEX     0x01000000
#define L_FORMAT_OCT     0x02000000
#define L_FORMAT_BIN     0x04000000
#define L_FORAMT_BSMASKS 0x07000000
#define L_FORMAT_LEFT    0x08000000
#define L_FORMAT_NEGSIGN 0x10000000
#define L_FORMAT_POSSIGN 0x20000000
#define L_FORMAT_BLKSIGN 0x40000000
#define L_FORMAT_SNMASKS 0x70000000
#define L_FORMAT_N0B0O0X 0x80000000
#define L_FORMAT_PRECISE 0x00800000
#define L_FORMAT_UPPER   0x00800000
#define L_FORMAT_REVERSE 0x00008000
#define L_FORMAT_MASKS   0xff808000

static l_umedit l_right_most_bit(l_umedit n) {
  return n & (-n);
}

static void l_log_write_to_file(l_logger* l) {
  l_write_file(&l->f, l->a, l->size);
  l->size = 0;
}

static void l_log_print(l_logger* l, l_strt s) {
  l_byte* p = 0;
  if (!s.start || s.len <= 0) return;
  if (l->capacity - l->size <= s.len) {
    l_log_write_to_file(l);
  }
  p = l->a + l->size;
  l->size += s.len;
  while (s.len-- > 0) {
    p[s.len] = s.start[s.len];
  }
}

static void l_log_print_reverse(l_logger* l, l_strt s) {
  if (!s.start || s.len <= 0) return;
  if (l->capacity - l->size <= s.len) {
    l_log_write_to_file(l);
  }
  while (s.len-- > 0) {
    l->a[l->size++] = s.start[s.len];
  }
}

static void l_log_string_fill_and_print(l_logger* log, l_rune* a, l_rune* p, l_umedit flags) {
  l_rune fill = l_cast(l_rune, flags & 0xff);
  int width = ((flags & 0x7f00) >> 8);
  int reverse = (flags & L_FORMAT_REVERSE) != 0;
  int left = (flags & L_FORMAT_LEFT) != 0;

  if (a >= p) return;

  if (width > p - a) {
    if (!fill) fill = ' ';

    if (reverse == left) {
      l_rune* end = a + width;

      /* [d,c,b,a,x,0, , ] -> [ , ,d,c,b,a,x,0]
         [0,x,a,b,c,d, , ] -> [ , ,0,x,a,b,c,d] */
      while (p > a) *(--end) = *(--p);

      /* fill # -> [#,#,d,c,b,a,x,0], [#,#,0,x,a,b,c,d] */
      while (end > a) *(--end) = fill;
      p = a + width;
    } else {
      /* fill # -> [d,c,b,a,x,0,#,#], [0,x,a,b,c,d,#,#] */
      while (width > p - a) *p++ = fill;
    }
  }

  if (reverse) {
    l_log_print_reverse(log, l_strt_e(a, p));
  } else {
    l_log_print(log, l_strt_e(a, p));
  }
}

static void l_log_string(l_logger* log, l_strt s, l_umedit flags) {
  int width = ((flags & 0x7f00) >> 8);
  if (s.len >= width) {
    l_log_print(log, s);
  } else {
    l_rune a[128];
    l_copy_l(s.start, s.len, a);
    l_log_string_fill_and_print(log, a, a + s.len, flags);
  }
}

static void l_log_format_string(l_logger* log, const void* a, l_umedit flags) {
  l_log_string(log, l_strt_c(a), flags);
}

static void l_log_format_strfrom(l_logger* log, const void* a, l_umedit flags) {
  l_log_string(log, *((const l_strt*)a), flags);
}

static void l_log_format_char(l_logger* log, l_ulong a, l_umedit flags) {
  l_rune ch = l_cast(l_rune, a&0xff);
  if ((flags & L_FORMAT_UPPER) && ch >= 'a' && ch <= 'z') {
    ch -= 32;
  }
  l_log_string(log, l_strt_l(&ch, 1), flags);
}

static void l_log_format_true(l_logger* log, l_ulong a, l_umedit flags) {
  if (a) {
    l_log_string(log, (flags & L_FORMAT_UPPER) ? l_literal_strt("TRUE") : l_literal_strt("true"), flags);
  } else {
    l_log_string(log, (flags & L_FORMAT_UPPER) ? l_literal_strt("FALSE") : l_literal_strt("false"), flags);
  }
}

static const l_rune* l_hex_runes[] = {
  l_rstr("0123456789abcdef"), l_rstr("0123456789ABCDEF")
};

static void l_log_format_ulong(l_logger* log, l_ulong n, l_umedit flags) {
  /* 64-bit unsigned int max value 18446744073709552046 (20 runes) */
  l_rune a[127];
  l_rune basechar = 0;
  l_rune* p = a;
  const l_rune* hex = 0;
  l_umedit precise = ((flags & 0x7f0000) >> 16);
  l_umedit base = 0;
  l_umedit sign = 0;

  flags &= L_FORMAT_MASKS;
  base = (flags & L_FORAMT_BSMASKS);

  switch (l_right_most_bit(base)) {
  case 0:
    *p++ = (n % 10) + '0';
    while ((n /= 10)) {
      *p++ = (n % 10) + '0';
    }
    break;

  case L_FORMAT_HEX:
    hex = l_hex_runes[(flags & L_FORMAT_UPPER) != 0];
    *p++ = hex[n & 0x0f];
    while ((n >>= 4)) {
      *p++ = hex[n & 0x0f];
    }
    if (!(flags & L_FORMAT_N0B0O0X)) {
      basechar = (flags & L_FORMAT_UPPER) ? 'X' : 'x';
    }
    flags &= (~L_FORMAT_SNMASKS);
    break;

  case L_FORMAT_OCT:
    *p++ = (n & 0x07) + '0';
    while ((n >>= 3)) {
      *p++ = (n & 0x07) + '0';
    }
    if (!(flags & L_FORMAT_N0B0O0X)) {
      basechar = (flags & L_FORMAT_UPPER) ? 'O' : 'o';
    }
    flags &= (~L_FORMAT_SNMASKS);
    break;

  case L_FORMAT_BIN:
    *p++ = (n & 0x01) + '0';
    while ((n >>= 1)) {
      *p++ = (n & 0x01) + '0';
    }
    if (!(flags & L_FORMAT_N0B0O0X)) {
      basechar = (flags & L_FORMAT_UPPER) ? 'B' : 'b';
    }
    flags &= (~L_FORMAT_SNMASKS);
    break;

  default:
    break;
  }

  while (precise > (p - a)) {
    *p++ = '0';
  }

  if (basechar) {
    *p++ = basechar;
    *p++ = '0';
  }

  sign = (flags & L_FORMAT_SNMASKS);
  if (sign) {
    switch (l_right_most_bit(sign)) {
    case L_FORMAT_NEGSIGN:
      *p++ = '-';
      break;
    case L_FORMAT_POSSIGN:
      *p++ = '+';
      break;
    case L_FORMAT_BLKSIGN:
      *p++ = ' ';
      break;
    default:
      break;
    }
  }

  l_log_string_fill_and_print(log, a, p, flags | L_FORMAT_REVERSE);
}

static void l_log_format_long(l_logger* log, l_int a, l_umedit flags) {
  l_ulong n = a;
  if (a < 0) {
    n = (-a);
    flags |= L_FORMAT_NEGSIGN;
  }
  l_log_format_ulong(log, n, flags);
}

static void l_log_print_ulong(l_ulong n, l_rune* p) {
  (void)n;
  (void)p;
}

static void l_log_print_fraction(l_ulong n, l_ulong intmasks, l_rune* p) {
  (void)n;
  (void)p;
  (void)intmasks;
} 

static void l_log_format_float(l_logger* log, l_value v, l_umedit flags) {
  l_rune a[127];
  l_rune* p = a;
  l_ulong fraction = 0;
  l_ulong mantissa = 0;
  l_ulong intmasks = 0;
  int exponent = 0;
  int negative = 0;
  (void)log;
  (void)flags;

  /**
   * Floating Point Components
   * |                  | Sign   | Exponent   | Fraction   | Bias
   * |----              |-----   | -----      |  ----      | ----
   * | Single Precision | 1 [31] |  8 [30-23] | 23 [22-00] | 127
   * | Double Precision | 1 [63] | 11 [62-52] | 52 [51-00] | 1023
   * ------------------------------------------------------------
   * Sign - 0 positive, 1 negative
   * Exponent - represent both positive and negative exponents
   *          - the ectual exponent = Exponent - (127 or 1023)
   *          - exponents of -127/-1023 (all 0s) and 128/1024 (255/2047, all 1s) are reserved for special numbers
   * Mantissa - stored in normalized form, this basically puts the radix point after the first non-zero digit
   *          - the mantissa has effectively 24/53 bits of resolution, by way of 23/52 fraction bits: 1.Fraction
   */

  negative = (v.u & 0x8000000000000000) != 0;
  exponent = (v.u & 0x7ff0000000000000) >> 52;
  fraction = (v.u & 0x000fffffffffffff);
  if (exponent == 0 && fraction == 0) {
    if (negative) *p++ = '-';
    *p++ = '0'; *p++ = '.'; *p++ = '0';
  } else if (exponent == 0x00000000000007ff) {
    if (fraction == 0) {
      if (negative) *p++ = '-';
      *p++ = 'I'; *p++ = 'N'; *p++ = 'F'; *p++ = 'I'; *p++ = 'N'; *p++ = 'I'; *p++ = 'T'; *p++ = 'Y';
    } else {
      *p++ = 'N'; *p++ = 'A'; *p++ = 'N';
    }
  } else {
    exponent = exponent - 1023;
    mantissa = 0x0010000000000000 | fraction;
    intmasks = 0xfff0000000000000;
    /*                         1.fraction
        [ , , , | , , , | , , ,1|f,r,a,c,t,i,o,n,n,n,...]
        <----------- 12 --------|-------- 52 ----------->    */
    if (exponent < 0) {
      /* only have fraction part */
      if (exponent < -8) {
        intmasks = 0xf000000000000000;
        exponent += 8; /* 0000.00000001fraction * 2^exponent */
        mantissa >>= (-exponent);
      } else {
        intmasks <<= (-exponent);
      }
      *p++ = '0';
      *p++ = '.';
      l_log_print_fraction(mantissa, intmasks, p);
    } else {
      if (exponent >= 52) {
        /* only have integer part */
        if (exponent <= 63) { /* 52 + 11 */
          mantissa <<= (exponent - 52);
          l_log_print_ulong(mantissa, p);
          *p++ = '.';
          *p++ = '0';
        } else {
          exponent -= 52;
          l_log_print_ulong(mantissa, p);
          *p++ = '*';
          *p++ = '2';
          *p++ = '^';
          l_log_print_ulong(exponent, p);
        }
      } else {
        /* have integer part and fraction part */
        intmasks >>= exponent;
        l_log_print_ulong((mantissa & intmasks) >> (52 - exponent), p);
        *p++ = '.';
        l_log_print_fraction(mantissa & (~intmasks), intmasks, p);
      }
    }
  }

  /* todo */
}

static int l_log_level = 2;

void l_set_log_level(int level) {
  l_log_level = level;
}

int l_get_log_level() {
  return l_log_level;
}

void l_assert_func_impl(int pass, const void* expr, const void* fileline) {
  if (pass) {
    l_logger_func_2("42[D] ", "%sassert pass: %s", lp(fileline), lp(expr));
  } else {
    l_logger_func_2("02[E] ", "%sassert fail: %s", lp(fileline), lp(expr));
  }
}

void l_logger_func_impl(const void* tag, const void* fmt, ...) {
  int level, numofargs;
  int fmtcnt = 0;
  const l_rune* start = 0;
  const l_rune* cur = 0;
  l_umedit flags = 0, width = 0;
  int fmtend = 0, ch = 0;
  l_logger* log = 0;
  va_list vl;

  level = l_rstr(tag)[0] - '0';
  numofargs = l_rstr(tag)[1] - '0';

  if (level < 0 || level > l_log_level) {
    return;
  }

  log = l_thread_logger();
  l_log_print(log, l_strt_c(l_rstr(tag) + 2));
  if (numofargs <= 0) {
    l_log_print(log, l_strt_c(fmt));
    return;
  }

  start = l_rstr(fmt);
  cur = start;

  va_start(vl, fmt);

  while (*cur) {
    if (*cur != '%') {
      ++cur;
      continue;
    }

    l_log_print(log, l_strt_e(start, cur));
    start = cur;
    if (fmtcnt >= numofargs) {
      break;
    }

    /**
     * s - const void*  // ?x print as hex, ?w treat as l_strt*
     * f - double
     * u - l_ulong
     * d - l_long
     * w - l_strt*
     * c - print as char
     * t - print 1 or 0
     * p - print as pointer
     * b - bin
     * o - oct
     * x - hex
     * formats:
     * base - (b)in (o)ct (h)ex
     * sign - ( )pace (+) (z) dont print 0b 0o 0x prefix
     * justify - (l)eft, default is right
     * width - 1 ~ 2 digit
     * precision - (.) and 1 ~ 2 digit
     * fill - fill to reach the width length
     */

    /* '%' is found */
    flags = 0;

ParseFormat:

    ++cur;
    switch (*cur) {
    case ' ':
      flags |= L_FORMAT_BLKSIGN;
      goto ParseFormat;

    case '+':
      flags |= L_FORMAT_POSSIGN;
      goto ParseFormat;

    case '.':
      flags |= L_FORMAT_PRECISE;
      goto ParseFormat;

    case 'l': case 'L':
      flags |= L_FORMAT_LEFT;
      goto ParseFormat;

    case 'z': case 'Z':
      flags |= L_FORMAT_N0B0O0X;
      goto ParseFormat;

    case '0': case '~': case '-': case '=': case '#':
      flags |= *cur; /* fill rune */
      goto ParseFormat;

    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      width = *cur - '0';
      ch = *(cur + 1);
      if (ch >= '0' && ch <= '9') {
        width = width * 10 + ch - '0';
        ++cur;
      } else {
        /* next char is not a digit */
        goto ParseFormat;
      }
      /* find 2-digit, ignore next extra digits */
      while ((ch = *(cur + 1)) >= '0' && ch <= '9') {
        ++cur;
      }
      /* current char is digit, and next is not */
      if (flags & L_FORMAT_PRECISE) {
        flags |= (width << 16);
      } else {
        flags |= (width << 8);
      }
      goto ParseFormat;

    case 'S':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 's':
      ++fmtcnt;
      l_log_format_string(log, va_arg(vl, l_value).p, flags);
      break;

    case 'f': case 'F':
      ++fmtcnt;
      l_log_format_float(log, va_arg(vl, l_value), flags);
      break;

    case 'u': case 'U':
      ++fmtcnt;
      l_log_format_ulong(log, va_arg(vl, l_value).u, flags);
      break;

    case 'd': case 'D':
      ++fmtcnt;
      l_log_format_long(log, va_arg(vl, l_value).d, flags);
      break;

    case 'w': case 'W':
      ++fmtcnt;
      l_log_format_strfrom(log, va_arg(vl, l_value).p, flags);
      break;

    case 'C':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'c':
      ++fmtcnt;
      l_log_format_char(log, va_arg(vl, l_value).u, flags);
      break;

    case 'T':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 't':
      ++fmtcnt;
      l_log_format_true(log, va_arg(vl, l_value).u, flags);
      break;

    case 'B':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'b':
      flags |= L_FORMAT_BIN;
      ++fmtcnt;
      l_log_format_ulong(log, va_arg(vl, l_value).u, flags);
      break;

    case 'O':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'o':
      flags |= L_FORMAT_OCT;
      ++fmtcnt;
      l_log_format_ulong(log, va_arg(vl, l_value).u, flags);
      break;

    case 'X': case 'P':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'x': case 'p':
      flags |= L_FORMAT_HEX;
      ++fmtcnt;
      l_log_format_ulong(log, va_arg(vl, l_value).u, flags);
      break;

    case 0:
      fmtend = 1;
      break;

    case '%':
      /* find the 2nd '%' */
      if (cur - start == 1) {
        start = cur++;
        /* %%abc - continue at 'a' */
      } else {
        /* %z%abc - continue at '%' */
        ++fmtcnt; /* skip this invalid fmt */
        va_arg(vl, l_value); /* skip the arg */
      }
      continue;
    default:
      /* %l+yabc - invalid format char 'y' */
      ++cur; /* continue at 'a' */
      ++fmtcnt; /* skip this invalid fmt */
      va_arg(vl, l_value); /* skip the arg */
      continue;
    }

    if (fmtend) {
      break;
    }

    /* current fmt is handled */
    ++cur;
    start = cur;
  }

  va_end(vl);

  if (start < cur) {
    l_log_print(log, l_strt_e(start, cur));
  }
}

void l_linknode_init(l_linknode* node) {
  node->next = node->prev = node;
}

int l_linknode_is_empty(l_linknode* node) {
  return (node->next == node);
}

void l_linknode_insert_after(l_linknode* node, l_linknode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
  newnode->prev = node;
  newnode->next->prev = newnode;
}

l_linknode* l_linknode_remove(l_linknode* node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  return node;
}

void l_smplnode_init(l_smplnode* node) {
  node->next = node;
}

int l_smplnode_is_empty(l_smplnode* node) {
  return node->next == node;
}

void l_smplnode_insert_after(l_smplnode* node, l_smplnode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
}

l_smplnode* l_smplnode_remove_next(l_smplnode* node) {
  l_smplnode* p = node->next;
  node->next = p->next;
  return p;
}

void l_squeue_init(l_squeue* self) {
  l_smplnode_init(&self->head);
  self->tail = &self->head;
}

void l_squeue_push(l_squeue* self, l_smplnode* newnode) {
  l_smplnode_insert_after(self->tail, newnode);
  self->tail = newnode;
}

void l_squeue_push_queue(l_squeue* self, l_squeue* q) {
  if (l_squeue_is_empty(q)) return;
  self->tail->next = q->head.next;
  self->tail = q->tail;
  self->tail->next = &self->head;
  l_squeue_init(q);
}

int l_squeue_is_empty(l_squeue* self) {
  return (self->head.next == &self->head);
}

l_smplnode* l_squeue_pop(l_squeue* self) {
  l_smplnode* node = 0;
  if (l_squeue_is_empty(self)) {
    return 0;
  }
  node = l_smplnode_remove_next(&self->head);
  if (node == self->tail) {
    self->tail = &self->head;
  }
  return node;
}

void l_dqueue_init(l_dqueue* self) {
  l_linknode_init(&self->head);
}

void l_dqueue_push(l_dqueue* self, l_linknode* newnode) {
  l_linknode_insert_after(&self->head, newnode);
}

void l_dqueue_push_queue(l_dqueue* self, l_dqueue* q) {
  l_linknode* tail = 0;
  if (l_dqueue_is_empty(q)) return;
  /* chain self's tail with q's first element */
  tail = self->head.prev;
  tail->next = q->head.next;
  q->head.next->prev = tail;
  /* chain q's tail with self's head */
  tail = q->head.prev;
  tail->next = &self->head;
  self->head.prev = tail;
  /* init q to empty */
  l_dqueue_init(q);
}

int l_dqueue_is_empty(l_dqueue* self) {
  return l_linknode_is_empty(&self->head);
}

l_linknode* l_dqueue_pop(l_dqueue* self) {
  if (l_dqueue_is_empty(self)) return 0;
  return l_linknode_remove(self->head.prev);
}

void l_priorq_init(l_priorq* self, int (*less)(void*, void*)) {
  l_linknode_init(&self->node);
  self->less = less;
}

int l_priorq_is_empty(l_priorq* self) {
  return l_linknode_is_empty(&self->node);
}

void l_priorq_push(l_priorq* self, l_linknode* elem) {
  l_linknode* head = &(self->node);
  l_linknode* cur = head->next;
  while (cur != head && self->less(cur, elem)) {
    cur = cur->next;
  }
  /* insert elem before current node  or the head node */
  cur->prev->next = elem;
  elem->prev = cur->prev;
  cur->prev = elem;
  elem->next = cur;
}

void l_priorq_remove(l_priorq* self, l_linknode* elem) {
  if (elem == &(self->node)) return;
  l_linknode_remove(elem);
}

l_linknode* l_priorq_pop(l_priorq* self) {
  l_linknode* head;
  l_linknode* first;
  if (l_priorq_is_empty(self)) {
    return 0;
  }
  head = &(self->node);
  first = head->next;
  head->next = first->next;
  first->next->prev = head;
  return first;
}

void l_mmheap_init(l_mmheap* self, int (*less)(void*, void*), int initsize) {
  self->size = self->capacity = 0;
  self->less = less;
  if (initsize > l_max_rdwr_size) {
    self->a = 0;
    l_loge_1("large %d", ld(initsize));
    return;
  }
  if (initsize < 8) initsize = 8;
  self->a = l_raw_calloc(initsize*sizeof(void*));
  self->capacity = initsize;
}

void l_mmheap_free(l_mmheap* self) {
  if (self->a) {
    l_raw_free(self->a);
    self->a = 0;
  }
}

static l_umedit ll_left(l_umedit n) {
  return n*2 + 1; /* right is left + 1 */
}

static l_umedit ll_parent(l_umedit n) {
  return (n == 0 ? 0 : (n - 1) / 2);
}

static int ll_has_child(l_mmheap* self, l_umedit i) {
  return self->size > ll_left(i);
}

static int ll_less_than(l_mmheap* self, l_umedit i, l_umedit j) {
  return self->less((void*)self->a[i], (void*)self->a[j]);
}

static l_umedit ll_min_child(l_mmheap* self, l_umedit i) {
  if (self->size & 0x01) {
    /* have two children */
    l_umedit left = ll_left(i);
    if (ll_less_than(self, left, left+1)) {
      return left;
    }
    return left+1;
  }
  /* only have left child */
  return ll_left(i);
}

static void ll_add_tail_elem(l_mmheap* self) {
  l_umedit i = self->size;
  while (i != 0) {
    l_umedit pa = ll_parent(i);
    if (ll_less_than(self, i, pa)) {
      l_uint temp = self->a[i];
      self->a[i] = self->a[pa];
      self->a[pa] = temp;
      i = pa;
      continue;
    }
    break;
  }
}

void l_mmheap_add(l_mmheap* self, void* elem) {
  if (self->capacity == 0 || elem == 0) {
    l_loge_s("mmheap_add invalid argument");
    return;
  }
  if (self->size >= self->capacity) {
    self->a = l_raw_realloc(self->a, self->capacity, self->capacity*2);
    self->capacity *= 2;
  }
  self->a[self->size++] = (l_uint)elem;
  ll_add_tail_elem(self);
}

void* l_mmheap_del(l_mmheap* self, l_umedit i) {
  void* elem = 0;
  if (i >= self->size) {
    return 0;
  }

  elem = (void*)self->a[i];
  self->a[i] = self->a[self->size-1];

  while (ll_has_child(self, i)) {
    l_umedit child = ll_min_child(self, i);
    l_uint temp = self->a[i];
    self->a[i] = self->a[child];
    self->a[child] = temp;
    i = child;
  }

  return elem;
}

void l_core_test() {
  char buffer[] = "012345678";
  char* a = buffer;
  l_strt strt = {0};
  l_strt* pstr = &strt;
  l_byte bytes[4] = {1};
#if defined(L_BUILD_DEBUG)
  l_logd_s("L_BUILD_DEBUG true");
#else
  l_logd_s("L_BUILD_DEBUG false");
#endif
  /* struct/array init */
  l_assert(strt.start == 0);
  l_assert(strt.len == 0);
  pstr->start = l_rstr(buffer);
  l_assert(*pstr->start == '0');
  l_assert(*pstr->start == *(pstr->start));
  l_assert(&pstr->start == &(pstr->start));
  l_assert(bytes[0] == 1);
  l_assert(bytes[1] == 0);
  l_assert(bytes[2] == 0);
  l_assert(bytes[3] == 0);
  /* type size */
  l_assert(sizeof(l_byte) == 1);
  l_assert(sizeof(l_sbyte) == 1);
  l_assert(sizeof(l_short) == 2);
  l_assert(sizeof(l_ushort) == 2);
  l_assert(sizeof(l_medit) == 4);
  l_assert(sizeof(l_umedit) == 4);
  l_assert(sizeof(l_long) == 8);
  l_assert(sizeof(l_ulong) == 8);
  l_assert(sizeof(l_uint) == sizeof(void*));
  l_assert(sizeof(l_int) == sizeof(void*));
  l_assert(sizeof(int) >= 4);
  l_assert(sizeof(char) == 1);
  l_assert(sizeof(float) == 4);
  l_assert(sizeof(double) == 8);
  l_assert(sizeof(size_t) >= 4);
  l_assert(sizeof(void*) >= 4);
  l_assert(sizeof(l_float) == 4 || sizeof(l_float) == 8);
  l_assert(sizeof(l_eight) == 8);
  l_assert(sizeof(l_mutex) >= L_MUTEX_SIZE);
  l_assert(sizeof(l_rwlock) >= L_RWLOCK_SIZE);
  l_assert(sizeof(l_condv) >= L_CONDV_SIZE);
  l_assert(sizeof(l_thrkey) >= L_THKEY_SIZE);
  l_assert(sizeof(l_thrid) >= L_THRID_SIZE);
  /* value limit */
  l_assert(l_max_ubyte == 255);
  l_assert(l_max_sbyte == 127);
  l_assert(l_min_sbyte == -127-1);
  l_assert(l_cast(l_byte, l_min_sbyte) == 0x80);
  l_assert(l_cast(l_byte, l_min_sbyte) == 128);
  l_assert(l_max_ushort == 65535);
  l_assert(l_max_short == 32767);
  l_assert(l_min_short == -32767-1);
  l_assert(l_cast(l_ushort, l_min_short) == 32768);
  l_assert(l_cast(l_ushort, l_min_short) == 0x8000);
  l_assert(l_max_umedit == 4294967295);
  l_assert(l_max_medit == 2147483647);
  l_assert(l_min_medit == -2147483647-1);
  l_assert(l_cast(l_umedit, l_min_medit) == 2147483648);
  l_assert(l_cast(l_umedit, l_min_medit) == 0x80000000);
  l_assert(l_max_ulong == 18446744073709551615ull);
  l_assert(l_max_long == 9223372036854775807ull);
  l_assert(l_min_long == -9223372036854775807-1);
  l_assert(l_cast(l_ulong, l_min_long) == 9223372036854775808ull);
  l_assert(l_cast(l_ulong, l_min_long) == 0x8000000000000000ull);
  /* copy test */
  l_copy_l(a, 1, a+1);
  l_assert(a[1] == '0'); a[1] = '1';
  l_copy_l(a+3, 1, a+2);
  l_assert(a[2] == '3'); a[2] = '2';
  l_copy_l(a, 4, a+2);
  l_assert(a[2] == '0');
  l_assert(a[3] == '1');
  l_assert(a[4] == '2');
  l_assert(a[5] == '3');
}
