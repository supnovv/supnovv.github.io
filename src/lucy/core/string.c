#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <float.h>
#include "lucycore.h"

int l_strt_contain(l_strt s, int ch) {
  while (s.start < s.end) {
    if (*s.start++ == ch) return true;
  }
  return false;
}

int l_strt_equal(l_strt l, l_strt r) {
  if (l.end - l.start != r.end - r.start) return false;
  while (l.start < l.end) {
    if (*l.start++ != *r.start++) return false;
  }
  return true;
}

static l_byte* l_strbuf_cstr(l_strbuf* self) {
  return (l_byte*)(self + 1);
}

static l_int l_strbuf_capacity(l_strbuf* self) {
  return self->bsize - sizeof(l_strbuf);
}

l_string l_thread_create_string(l_thread* thread, l_int initsize) {
  return l_thread_create_limited_string(thread, initsize, 0);
}

l_string l_thread_create_string_from(l_thread* thread, l_strt from) {
  return l_thread_create_limited_string_from(thread, from, 0);
}

l_string l_empty_string() {
  return l_create_string(0);
}

l_string l_create_string(l_int initsize) {
  return l_thread_create_string(0, initsize);
}

l_string l_create_string_from(l_strt from) {
  return l_thread_create_string_from(0, from);
}

l_string l_create_limited_string(l_int initsize, l_int maxlimit) {
  return l_thread_create_limited_string(0, initsize, maxlimit);
}

l_string l_create_limited_string_from(l_strt from, l_int maxlimit) {
  return l_thread_create_limited_string_from(0, from, maxlimit);
}

void l_string_free(l_string* self) {
  l_thread_string_free(0, self);
}

void l_string_clear(l_string* self) {
  l_strbuf* p = self->b;
  *l_strbuf_cstr(p) = 0;
  p->size = 0;
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
#define L_STRING_PRINT_LOG 0x01

static l_umedit l_right_most_bit(l_umedit n) {
  return n & (-n);
}

static int l_log_level = 2;

static void l_write_log_to_file(l_string* self) {
  l_strbuf* b = self->b;
  if (b->size > 0) {
    l_thread* thread = (l_thread*)((l_byte*)self - offsetof(l_thread, log));
    l_write_file(&thread->logfile, l_strbuf_cstr(b), b->size);
    if (l_log_level >= 4 && thread->logfile.stream != stdout && thread->logfile.stream != stderr) {
      l_filestream file = {stdout};
      l_write_file(&file, l_strbuf_cstr(b), b->size);
    }
  }
  *(l_strbuf_cstr(b)) = 0;
  b->size = 0;
}

void l_thread_flush_log(l_thread* self) {
  l_write_log_to_file(&self->log);
}

static void l_string_format_out(l_string* self, l_strt s) {
  if (self->b->flags & L_STRING_PRINT_LOG) {
    l_strbuf* b = self->b;
    l_int len = s.end - s.start;
    if (len <= 0) return;

    if (l_string_remain(self) < len + 1) { /* 1 is for zero-terminated char */
      l_write_log_to_file(self);

      if (l_string_remain(self) < len + 1) {
        l_thread* thread = (l_thread*)((l_byte*)self - offsetof(l_thread, log));
        l_write_file(&thread->logfile, s.start, len);
        l_logw_2("buffer too small %d len %d", ld(l_string_capacity(self)), ld(len));
        return;
      }
    }

    l_copy_from(s, l_string_end(self));
    b->size += len;
    *(l_string_end(self)) = 0;

  } else {
    l_string_append(self, s);
  }
}

static void l_string_format_out_reverse(l_string* self, l_strt s) {
  l_strbuf* b = self->b;
  l_rune* dest = 0;
  l_int len = s.end - s.start;
  if (len <= 0) return;

  if (self->b->flags & L_STRING_PRINT_LOG) {

    if (l_string_remain(self) < len + 1) {
      l_write_log_to_file(self);

      if (l_string_remain(self) < len + 1) {
        l_thread* thread = (l_thread*)((l_byte*)self - offsetof(l_thread, log));
        l_write_file(&thread->logfile, s.start, len);
        l_logw_2("buffer too small %d len %d", ld(l_string_capacity(self)), ld(len));
        return;
      }
    }

  } else {
    if (!l_string_ensure_remain(self, len + 1)) {
      l_loge_1("len %d", ld(len));
      return;
    }
  }

  dest = l_string_end(self);
  while (s.start < s.end--) {
    *dest++ = *s.end;
  }

  *dest = 0;
  b->size += len;
}

static void l_string_format_fill_and_out(l_string* self, l_rune* a, l_rune* p, l_umedit flags) {
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
    l_string_format_out_reverse(self, l_strt_e(a, p));
  } else {
    l_string_format_out(self, l_strt_e(a, p));
  }
}

static void l_string_format_string(l_string* self, l_strt s, l_umedit flags) {
  int width = ((flags & 0x7f00) >> 8);
  l_int len = s.end - s.start;
  if (len >= width) {
    l_string_format_out(self, s);
  } else {
    l_rune a[128];
    memcpy(a, s.start, len);
    l_string_format_fill_and_out(self, a, a + len, flags);
  }
}

static void l_string_format_char(l_string* self, l_ulong a, l_umedit flags) {
  l_rune ch = l_cast(l_rune, a&0xff);
  if ((flags & L_FORMAT_UPPER) && ch >= 'a' && ch <= 'z') {
    ch -= 32;
  }
  l_string_format_string(self, l_strt_l(&ch, 1), flags);
}

static void l_string_format_true(l_string* self, l_ulong a, l_umedit flags) {
  if (a) {
    l_string_format_string(self, (flags & L_FORMAT_UPPER) ? l_literal_strt("TRUE") : l_literal_strt("true"), flags);
  } else {
    l_string_format_string(self, (flags & L_FORMAT_UPPER) ? l_literal_strt("FALSE") : l_literal_strt("false"), flags);
  }
}

/**
 * [non-zero] pritable charater
 * [00111010] G ~ Z   0x3a
 * [00111011] g ~ z   0x3b
 * [00111101] 0 ~ 9   0x3d
 * [00111110] A ~ F   0x3e
 * [00111111] a ~ f   0x3f
 * [00110000] _       0x30
 * [00100000] -       0x20
 * [0000XX1X] letter                 (ch & 0x02)
 * [0000XX10] upper letter           (ch & 0x03) == 2
 * [0000XX11] lower letter           (ch & 0x03) == 3
 * [0000X1XX] hex digit              (ch & 0x04)
 * [00001XXX] alphanum               (ch & 0x08)
 * [XXX1XXXX] alphanum and _         (ch & 0x10)
 * [XX1XXXXX] alphanum and _ and -   (ch & 0x20)
 */
const l_rune l_rune_class_table[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, 0x80,0x80,0x80,0x80,0x80,0x20,0x80,0x80, 0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d, 0x3d,0x3d,0x80,0x80,0x80,0x80,0x80,0x80, /* (20) - (3d) 0 ~ 9 */
  0x80,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3a, 0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a, 0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a, 0x3a,0x3a,0x3a,0x80,0x80,0x80,0x80,0x30, /* (3e,3a) A ~ Z (30) _ */
  0x80,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3b, 0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b, 0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b, 0x3b,0x3b,0x3b,0x80,0x80,0x80,0x80,0x00, /* (3f,3b) a ~ z */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const l_rune* l_hex_runes[] = {
  l_rstr("0123456789abcdef"), l_rstr("0123456789ABCDEF")
};

l_int l_string_parse_dec(l_strt s) {
  l_int times = 1;
  l_int value = 0;
  const l_rune* start = 0;
  const l_rune* end = 0;
  int negative = false;

  while (s.start < s.end) {
    if (*s.start >= '0' && *s.start <= '9') break;
    if (*s.start == '-') negative = true;
    ++s.start;
  }

  start = s.start;
  while (s.start < s.end) {
    if (*s.start < '0' || *s.start > '9') break;
    ++s.start;
  }

  end = s.start;
  while (start < end--) {
    value += *end * times;
    times *= 10;
  }

  return (negative ? -value : value);
}

l_int l_string_parse_hex(l_strt s) {
  l_int times = 1;
  l_int value = 0;
  const l_rune* start = 0;
  const l_rune* end = 0;
  int negative = false;

  while (s.start < s.end) {
    if (l_check_is_hex_digit(*s.start)) break;
    if (*s.start == '-') negative = true;
    ++s.start;
  }

  if (*s.start == '0' && s.start + 1 < s.end && (*(s.start + 1) == 'x' || *(s.start + 1) == 'X')) {
    s.start += 2;
    if (s.start >= s.end || !l_check_is_hex_digit(*s.start)) {
      return 0;
    }
  }

  start = s.start;
  while (s.start < s.end) {
    if (!l_check_is_hex_digit(*s.start)) break;
    ++s.start;
  }

  end = s.start;
  while (start < end--) {
    value += *end * times;
    times *= 16;
  }

  return (negative ? -value : value);
}


static void l_string_format_ulong(l_string* self, l_ulong n, l_umedit flags) {
  /* 64-bit unsigned int max value 18446744073709552046 (20 runes) */
  l_rune a[127];
  l_rune basechar = 0;
  l_rune* p = a;
  const l_rune* hex = 0;
  l_umedit precise = ((flags & 0x7f0000) >> 16);
  l_umedit base = 0;

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

  if (flags & L_FORMAT_NEGSIGN) *p++ = '-';
  else if (flags & L_FORMAT_POSSIGN) *p++ = '+';
  else if (flags & L_FORMAT_BLKSIGN) *p++ = ' ';

  l_string_format_fill_and_out(self, a, p, flags | L_FORMAT_REVERSE);
}

static void l_string_format_long(l_string* self, l_long a, l_umedit flags) {
  l_ulong n = a;
  if (a < 0) {
    n = (-a);
    flags |= L_FORMAT_NEGSIGN;
  }
  l_string_format_ulong(self, n, flags);
}

l_rune* l_string_print_ulong(l_ulong n, l_rune* p) {
  l_rune a[80];
  l_rune* s = a;

  *s++ = (n % 10) + '0';

  while ((n /= 10)) {
    *s++ = (n % 10) + '0';
  }

  while (s-- > a) {
    *p++ = *s;
  }

  return p;
}

static l_rune* l_string_print_fraction(double f, l_rune* p, int precise) {
  l_ulong ipart = 0;

  if (f < DBL_EPSILON) {
    *p++ = '0';
    return p;
  }

  if (precise == 0) precise = 80;
  while (f >= DBL_EPSILON && precise-- > 0) {
    ipart = l_cast(l_ulong, f * 10);
    *p++ = l_cast(l_rune, ipart + '0');
    f = f * 10 - ipart;
  }

  return p;
}

static void l_string_format_float(l_string* self, l_value v, l_umedit flags) {
  l_rune a[144];
  l_rune sign = 0;
  l_rune* p = a;
  l_rune* dot = 0;
  l_ulong fraction = 0;
  l_ulong mantissa = 0;
  int exponent = 0;
  int negative = 0;
  l_umedit precise = ((flags & 0x7f0000) >> 16);

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

  if (negative) sign = '-';
  else if (flags & L_FORMAT_POSSIGN) sign = '+';
  else if (flags & L_FORMAT_BLKSIGN) sign = ' ';

  if (exponent == 0 && fraction == 0) {
    if (sign) *p++ = sign;
    *p++ = '0'; *p++ = '.'; dot = p; *p++ = '0';
  } else if (exponent == 0x00000000000007ff) {
    if (fraction == 0) {
      if (sign) *p++ = sign;
      *p++ = 'I'; *p++ = 'N'; *p++ = 'F'; *p++ = 'I'; *p++ = 'N'; *p++ = 'I'; *p++ = 'T'; *p++ = 'Y';
    } else {
      if (flags & L_FORMAT_BLKSIGN) *p++ = ' ';
      *p++ = 'N'; *p++ = 'A'; *p++ = 'N';
    }
  } else {
    if (sign) *p++ = sign;
    if (negative) v.u &= 0x7fffffffffffffff;

    exponent = exponent - 1023;
    mantissa = 0x0010000000000000 | fraction;
    /* intmasks = 0xfff0000000000000; */
    /*                         1.fraction
        [ , , , | , , , | , , ,1|f,r,a,c,t,i,o,n,n,n,...]
        <----------- 12 --------|-------- 52 ----------->    */
    if (exponent < 0) {
      /* only have fraction part */
      #if 0
      if (exponent < -8) {
        intmasks = 0xf000000000000000;
        exponent += 8; /* 0000.00000001fraction * 2^exponent */
        mantissa >>= (-exponent); /* lose lower digits */
      } else {
        intmasks <<= (-exponent);
      }
      *p++ = '0'; dot = p; *p++ = '.';
      l_log_print_fraction(mantissa, intmasks, p);
      #endif
      *p++ = '0'; *p++ = '.'; dot = p;
      p = l_string_print_fraction(v.f, p, precise);
    } else {
      if (exponent >= 52) {
        /* only have integer part */
        if (exponent <= 63) { /* 52 + 11 */
          mantissa <<= (exponent - 52);
          p = l_string_print_ulong(mantissa, p);
          *p++ = '.'; dot = p; *p++ = '0';
        } else {
          exponent -= 63;
          mantissa <<= 11;
          p = l_string_print_ulong(mantissa, p);
          *p++ = '*'; *p++ = '2'; *p++ = '^';
          p = l_string_print_ulong(exponent, p);
        }
      } else {
        /* have integer part and fraction part */
        #if 0
        intmasks >>= exponent;
        l_log_print_ulong((mantissa & intmasks) >> (52 - exponent), p);
        *p++ = '.';
        l_log_print_fraction(mantissa & (~intmasks), intmasks, p);
        #endif
        l_ulong ipart = l_cast(l_ulong, v.f);
        p = l_string_print_ulong(ipart, p);
        *p++ = '.'; dot = p;
        p = l_string_print_fraction(v.f - ipart, p, precise);
      }
    }
  }

  if (dot && precise) {
    while (p - dot < precise) {
      *p++ = '0';
    }
  }

  l_string_format_fill_and_out(self, a, p, flags);
}

void l_set_log_level(int level) {
  l_log_level = level;
}

int l_get_log_level() {
  return l_log_level;
}

void l_assert_func_pass(const void* tag, const void* expr) {
  l_logger_func_1(tag, "assert pass: %s", lp(expr));
}

void l_assert_func_fail(const void* tag, const void* expr) {
  l_logger_func_1(tag, "assert fail: %s", lp(expr));
}

static const l_rune* l_string_format_a_value(l_string* self, const l_rune* start, const l_rune* end, l_value a) {
  l_umedit flags = 0; /* start pointer to '%' and next rune is not a '%' */
  const l_rune* cur = start;
  /**
   * s - const void*          ls(a) lp(a)
   * f - double               lf(a)
   * u - l_ulong              lu(a)
   * d - l_long               ld(a)
   * strt - l_strt*           lstrt(a)
   * strn - l_strn*           lstrn(a)
   * c - print as char        lc(a) ld(a)
   * t - print 1 or 0         lt(a) ld(a)
   * p - print as pointer     lp(a)
   * b - bin                  lb(a) lu(a)
   * o - oct                  lo(a) lu(a)
   * x - hex                  lx(a) lu(a)
   * formats:
   * base - (b)in (o)ct (h)ex
   * sign - ( )pace (+) (z) dont print 0b 0o 0x prefix
   * justify - (l)eft, default is right
   * width - 1 ~ 2 digit
   * precision - (.) and 1 ~ 2 digit
   * fill - fill to reach the width length
   */

  while (++cur < end) {
    switch (*cur) {
    case ' ':
      flags |= L_FORMAT_BLKSIGN;
      continue;

    case '+':
      flags |= L_FORMAT_POSSIGN;
      continue;

    case '.':
      flags |= L_FORMAT_PRECISE;
      continue;

    case 'l': case 'L':
      flags |= L_FORMAT_LEFT;
      continue;

    case 'z': case 'Z':
      flags |= L_FORMAT_N0B0O0X;
      continue;

    case '0': case '~': case '-': case '=': case '#':
      flags |= *cur; /* fill rune */
      continue;

    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
        l_umedit width = *cur - '0';
        l_rune ch = 0;
        if (cur + 1 < end && (ch = *(cur + 1)) >= '0' && ch <= '9') {
          /* cur + 1 is the 2nd digit */
          width = width * 10 + ch - '0';
          ++cur;
          /* skip next extra digits */
          while (cur + 1 < end && (ch = *(cur + 1)) >= '0' && ch <= '9') {
            ++cur;
          }
        }
        /* current char is digit, and next is not digit or end */
        if (flags & L_FORMAT_PRECISE) {
          flags |= (width << 16);
        } else {
          flags |= (width << 8);
        }
      }
      continue;

    case 'S':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 's':
      if (end - cur >= 4 && *(cur+1) == 't' && *(cur+2) == 'r' && (*(cur+3) == 't' || *(cur+3) == 'n')) {
        if (*(cur+3) == 't') {
          l_string_format_string(self, *((l_strt*)a.p), flags);
        } else {
          l_string_format_string(self, l_strn_to_strt((l_strn*)a.p), flags);
        }
        return cur + 4;
      }
      l_string_format_string(self, l_strt_c(a.p), flags);
      return cur + 1;

    case 'f': case 'F':
      l_string_format_float(self, a, flags);
      return cur + 1;

    case 'u': case 'U':
      l_string_format_ulong(self, a.u, flags);
      return cur + 1;

    case 'd': case 'D':
      l_string_format_long(self, a.d, flags);
      return cur + 1;

    case 'C':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'c':
      l_string_format_char(self, (int)a.u, flags);
      return cur + 1;

    case 'T':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 't':
      l_string_format_true(self, (int)a.u, flags);
      return cur + 1;

    case 'B':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'b':
      flags |= L_FORMAT_BIN;
      l_string_format_ulong(self, a.u, flags);
      return cur + 1;

    case 'O':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'o':
      flags |= L_FORMAT_OCT;
      l_string_format_ulong(self, a.u, flags);
      return cur + 1;

    case 'X': case 'P':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'x': case 'p':
      flags |= L_FORMAT_HEX;
      l_string_format_ulong(self, a.u, flags);
      return cur + 1;

    default:
      break;
    }

    break;
  }

  l_string_format_out(self, l_strt_e(start, end));
  return 0;
}

static int l_string_format_v_impl(l_string* self, const l_rune* fmt, va_list vl) {
  int nfmts = 0;
  int nargs = self->b->nargs;
  const l_rune* cur = fmt;
  const l_rune* end = cur + strlen((char*)fmt);

  if (nargs <= 0 || nargs > 9) {
    l_string_format_out(self, l_strt_c(fmt));
    return 0;
  }

  while (cur < end) {
    if (*cur != '%') {
      ++cur;
      continue;
    }

    l_string_format_out(self, l_strt_e(fmt, cur));

    if (cur + 1 < end && *(cur + 1) == '%') {
      fmt = cur + 1;
      cur = cur + 2;
      continue;
    }

    /* cur is '%' and next is not '%' or end */
    if (!(cur = l_string_format_a_value(self, cur, end, va_arg(vl, l_value)))) {
      break;
    }

    fmt = cur;
    ++nfmts;

    if (nfmts >= nargs) {
      break;
    }
  }

  if (fmt < end) {
    l_string_format_out(self, l_strt_e(fmt, end));
  }

  return nfmts;
}

int l_string_format_n_impl(l_string* self, const void* fmt, int n, l_value* a) {
  int nfmts = 0;
  const l_rune* cur = 0;
  const l_rune* end = 0;
  const l_rune* beg = 0;

  if (!fmt) return 0;

  if (n <= 0 || !a) {
    l_string_format_out(self, l_strt_c(fmt));
    return 0;
  }

  cur = l_rstr(fmt);
  end = cur + strlen((char*)fmt);
  beg = cur;

  while (cur < end) {
    if (*cur != '%') {
      ++cur;
      continue;
    }

    l_string_format_out(self, l_strt_e(beg, cur));

    if (cur + 1 < end && *(cur + 1) == '%') {
      beg = cur + 1;
      cur = cur + 2;
      continue;
    }

    /* cur is '%' and next is not '%' or end */
    if (!(cur = l_string_format_a_value(self, cur, end, a[nfmts]))) {
      break;
    }

    beg = cur;
    ++nfmts;

    if (nfmts >= n) {
      break;
    }
  }

  if (beg < end) {
    l_string_format_out(self, l_strt_e(beg, cur));
  }

  return nfmts;
}

int l_string_format_impl(l_string* self, const void* fmt, ...) {
  int nfmts = 0;
  va_list vl;

  if (!fmt) return 0;

  va_start(vl, fmt);
  nfmts = l_string_format_v_impl(self, l_rstr(fmt), vl);
  va_end(vl);

  return nfmts;
}

void l_logger_func_impl(const void* tag, const void* fmt, ...) {
  int level = l_rstr(tag)[0] - '0';
  int nargs = l_rstr(tag)[1];
  l_thread* thread = 0;
  l_string* log = 0;
  va_list vl;

  if (level < 0 || level > l_log_level) return;

  thread = l_thread_self();
  if (!thread) {
    printf("%s00 %s ~\n", ((char*)tag) + 2, (char*)fmt);
    return;
  }

  log = &thread->log;
  log->b->flags |= L_STRING_PRINT_LOG;
  l_string_format_out(log, l_strt_c(l_rstr(tag) + 2));
  l_string_format_ulong(log, thread->index, (2 << 16));
  l_string_format_out(log, l_literal_strt(" "));

  if (!fmt) return;

  if (nargs == 'n') {
    va_start(vl, fmt);
    nargs = va_arg(vl, l_int);
    l_string_format_n_impl(log, fmt, nargs, va_arg(vl, l_value*));
    va_end(vl);
    return;
  }

  va_start(vl, fmt);
  log->b->nargs = nargs - '0';
  l_string_format_v_impl(log, l_rstr(fmt), vl);
  va_end(vl);

  l_string_format_out(log, l_strt_l(L_NEWLINE, L_NL_SIZE));
  if (l_log_level >= 4) l_write_log_to_file(log);
}

#define L_BLANK_MAX_LEN (3)
#define L_NUM_OF_SPACES (23)
#define L_NUM_OF_NEWLINES (6)
#define L_NUM_OF_BLANKS (29)

typedef struct {
  l_umedit m; /* string match this rune */
  l_umedit e; /* string ended with this rune */
} l_runeinfo;

typedef struct { /* 4 * 256 * 2 = 2048 (2KB) */
  l_runeinfo a[256];
} l_runetable;

static const l_strn l_blanks[] = {
  l_literal_strn("\x09"), /* \t */
  l_literal_strn("\x0b"), /* \v */
  l_literal_strn("\x0c"), /* \f */
  /* Zs 'Separator, Space' Category - www.fileformat.info/info/unicode/category/Zs/list.htm */
  l_literal_strn("\x20"), /* 0x20 space */
  l_literal_strn("\xc2\xa0"), /* 0xA0 no-break space */
  l_literal_strn("\xe1\x9a\x80"), /* 0x1680 ogham space mark */
  l_literal_strn("\xe2\x80\xaf"), /* 0x202F narrow no-break space */
  l_literal_strn("\xe2\x81\x9f"), /* 0x205F medium mathematical space */
  l_literal_strn("\xe3\x80\x80"), /* 0x3000 ideographic space (chinese blank character) */
  l_literal_strn("\xe2\x80\x80"), /* 0x2000 en quad */
  l_literal_strn("\xe2\x80\x81"), /* 0x2001 em quad */
  l_literal_strn("\xe2\x80\x82"), /* 0x2002 en space */
  l_literal_strn("\xe2\x80\x83"), /* 0x2003 em space */
  l_literal_strn("\xe2\x80\x84"), /* 0x2004 three-per-em space */
  l_literal_strn("\xe2\x80\x85"), /* 0x2005 four-per-em space */
  l_literal_strn("\xe2\x80\x86"), /* 0x2006 six-per-em space */
  l_literal_strn("\xe2\x80\x87"), /* 0x2007 figure space */
  l_literal_strn("\xe2\x80\x88"), /* 0x2008 punctuation space */
  l_literal_strn("\xe2\x80\x89"), /* 0x2009 thin space */
  l_literal_strn("\xe2\x80\x8a"), /* 0x200A hair space */
  /* byte order marks */
  l_literal_strn("\xfe\xff"),
  l_literal_strn("\xff\xfe"),
  l_literal_strn("\xef\xbb\xbf"),
  /* new lines */
  l_literal_strn("\x0a\x0d"), /* \n\r */
  l_literal_strn("\x0d\x0a"), /* \r\n */
  l_literal_strn("\x0a"), /* \r */
  l_literal_strn("\x0d"), /* \n */
  l_literal_strn("\xe2\x80\xa8"), /* line separator 0x2028 00100000_00101000 -> 1110'0010_10'000000_10'101000 (0xE280A8) */
  l_literal_strn("\xe2\x80\xa9")  /* paragraph separator 0x2029 00100000_00101001 */
};

static l_stringmap l_space_map; /* used to match a space */
static l_stringmap l_newline_map; /* used to match a newline */
static l_stringmap l_blank_map; /* used to match a blank, blank is a space or a newline */

const l_stringmap* l_string_space_map() {
  l_stringmap* map = &l_space_map;
  if (map->t) return map;
  *map = l_string_new_map(L_BLANK_MAX_LEN, l_blanks, L_NUM_OF_SPACES, true);
  return map;
}

const l_stringmap* l_string_newline_map() {
  l_stringmap* map = &l_newline_map;
  if (map->t) return map;
  *map = l_string_new_map(L_BLANK_MAX_LEN, l_blanks + L_NUM_OF_SPACES, L_NUM_OF_NEWLINES, true);
  return map;
}

const l_stringmap* l_string_blank_map() {
  l_stringmap* map = &l_blank_map;
  if (map->t) return map;
  *map = l_string_new_map(L_BLANK_MAX_LEN, l_blanks, L_NUM_OF_BLANKS, true);
  return map;
}

/* char classes
%a - all letters              %A - the complement of %a
%d - all digits               %D
%x - all hexadecimal digits   %X
%l - all lowercase letters    %L
%u - all uppercase letters    %U
%. - all chars
%% - '%'
%[ - '['
%] - ']'
%^ - '^'
[chars] - character set
[^chars] - the complement of [chars]
*/

void l_string_set_map(l_stringmap* self, const l_strn* str, l_int numofstr, int casesensitive) {
  int stridx = 0, charidx = 0;
  const l_rune* s = 0;
  l_runetable* t = (l_runetable*)(self->t);
  l_rune ch = 0;

  l_zero_l(self->t, sizeof(l_runetable) * self->size);

  for (; stridx < numofstr; ++stridx) {

    if (stridx + 1 > self->maxnumofstr) {
      l_loge_s("too many strings");
      break;
    }

    s = str[stridx].start;
    if (s == 0 || s[0] == 0) continue;

    charidx = 0;
    while ((ch = s[charidx])) {

      if (charidx + 1 > self->size) {
        l_loge_s("string too long");
        ++charidx;
        break;
      }

      /* the string match this char */
      t[charidx].a[ch].m |= (1 << stridx);
      if (!casesensitive) {
        if (ch >= 'a' && ch <= 'z') t[charidx].a[ch-32].m |= (1 << stridx);
        else if (ch >= 'A' && ch <= 'Z') t[charidx].a[ch+32].m |= (1 << stridx);
      }

      ++charidx;
    }

    /* the string ended with this char */
    ch = s[--charidx];
    t[charidx].a[ch].e |= (1 << stridx);
    if (!casesensitive) {
      if (ch >= 'a' && ch <= 'z') t[charidx].a[ch-32].e |= (1 << stridx);
      else if (ch >= 'A' && ch <= 'Z') t[charidx].a[ch+32].e |= (1 << stridx);
    }
  }
}

l_stringmap l_string_new_map(l_int maxstrlen, const l_strn* str, l_int numofstr, int casesensitive) {
  l_stringmap map = {0};
  if (maxstrlen <= 0 || str == 0) return map;
  map.t = (l_runetable*)l_raw_malloc(sizeof(l_runetable) * maxstrlen);
  map.size = maxstrlen;
  map.maxnumofstr = 32;
  l_string_set_map(&map, str, numofstr, casesensitive);
  return map;
}

void l_string_free_map(l_stringmap* self) {
  if (self->t == 0) return;
  l_raw_free(self->t);
  self->t = 0;
  self->size = 0;
}

static const l_rune* const l_string_too_short = (const l_rune* const)(l_int)(1);

static int l_power_of_two_bit_pos(l_umedit n) {
  switch (n) {
  case 0x00000001: return 0;
  case 0x00000002: return 1;
  case 0x00000004: return 2;
  case 0x00000008: return 3;
  case 0x00000010: return 4;
  case 0x00000020: return 5;
  case 0x00000040: return 6;
  case 0x00000080: return 7;
  case 0x00000100: return 8;
  case 0x00000200: return 9;
  case 0x00000400: return 10;
  case 0x00000800: return 11;
  case 0x00001000: return 12;
  case 0x00002000: return 13;
  case 0x00004000: return 14;
  case 0x00008000: return 15;
  case 0x00010000: return 16;
  case 0x00020000: return 17;
  case 0x00040000: return 18;
  case 0x00080000: return 19;
  case 0x00100000: return 20;
  case 0x00200000: return 21;
  case 0x00400000: return 22;
  case 0x00800000: return 23;
  case 0x01000000: return 24;
  case 0x02000000: return 25;
  case 0x04000000: return 26;
  case 0x08000000: return 27;
  case 0x10000000: return 28;
  case 0x20000000: return 29;
  case 0x40000000: return 30;
  case 0x80000000: return 31;
  default: break;
  }
  l_loge_s("invalid number");
  return 0;
}

/* return 0 doesn't match, -1 too short, >s match success */
const l_rune* l_string_match_ex(const l_stringmap* map, l_strt s, l_int* strid, l_int* matched_len) {
  l_umedit prevmatch = 0xFFFFFFFF, curmatch = 0;
  l_umedit matches = 0, headmatch = 0;
  l_runetable* t = 0;
  l_int i = 0, len = s.end - s.start;
  l_rune ch = 0;
  const l_rune* p = 0;

  /* example - (1) aabbcc (2) aabb (3) aa (4)zzbbcc
  (a) curmatch = 0b0111;
  (a) curmatch = 0b0111; t->e['a'] = 0b0100; headmatch = 0b0001;
  (b) curmatch = 0b0011;
  (b) curmatch = 0b0011; t->e['b'] = 0b1010; headmatch = 0b0001;
  (c) curmatch = 0b0001;
  (c) curmatch = 0b0001; t->e['c'] = 0b1001; headmatch = 0b0001;
  */

  if (len <= 0) {
    return l_string_too_short;
  }

  if (map->size < len) {
    len = map->size;
  }

  while (i < len) {
    t = ((l_runetable*)(map->t)) + i;
    ch = s.start[i];

    curmatch = prevmatch & t->a[ch].m;
    if (!curmatch) {
      if (p) {
        headmatch = l_right_most_bit(matches);
        goto MatchSuccess;
      }
      return 0;
    }

    if (t->a[ch].e & curmatch) {
      p = s.start + i + 1;
      matches = t->a[ch].e & curmatch;
      headmatch = l_right_most_bit(curmatch);
      if (matches & headmatch) {
        goto MatchSuccess;
      }
    }

    prevmatch = curmatch;
    ++i;
  }

  if (p) {
    headmatch = l_right_most_bit(matches);
    goto MatchSuccess;
  }

  return l_string_too_short; /* too short to match */

MatchSuccess:
  if (strid) *strid = l_power_of_two_bit_pos(headmatch);
  if (matched_len) *matched_len = p - s.start;
  return p;
}

const l_rune* l_string_match(const l_stringmap* map, l_strt s) {
  return l_string_match_ex(map, s, 0, 0);
}

/* match exactly n times, return 0 or l_string_too_short for fail */
const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, l_strt s) {
  const l_rune* e = s.start;
  int i = 0;
  while (i++ < n) {
    if ((e = l_string_match(map, s)) == 0 || e == l_string_too_short) return 0;
    s.start = e;
  }
  return e;
}

const l_rune* l_string_match_repeat(const l_stringmap* map, l_strt s) {
  const l_rune* e = 0;
  while ((e = l_string_match(map, s)) != 0 && e != l_string_too_short) {
    s.start = e;
  }
  return s.start;
}

/* return 0 - too short to match, otherwise success */
const l_rune* l_string_match_until(const l_stringmap* map, l_strt s, l_rune** last_match_start) {
  const l_rune* e = 0;
  while ((e = l_string_match(map, s)) == 0) { /* continue loop when unmatched */
    ++s.start;
  }
  if (last_match_start) *last_match_start = (l_rune*)s.start;
  return (e == l_string_too_short ? 0 : e);
}

const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, l_strt s, l_rune** first_non_space_pos) {
  const l_rune* e = 0;
  l_rune* last_match_start = 0;

  while ((e = l_string_match(l_string_space_map(), s)) != 0 && e != l_string_too_short) {
    s.start = e; /* skip space */
  }

  if (e == l_string_too_short) return 0; /* last space is not complete */
  if (first_non_space_pos) *first_non_space_pos = (l_rune*)s.start;
  if (!(e = l_string_match_until(map, s, &last_match_start))) {
    return last_match_start;
  }
  return e;
}

/* return 0 - match failed; otherwise success, strid and mlen are set */
const l_rune* l_string_skip_space_and_match(const l_stringmap* map, l_strt s, l_int* strid, l_int* matched_len) {
  const l_rune* e = 0;
  while ((e = l_string_match(l_string_space_map(), s)) != 0 && e != l_string_too_short) {
    s.start = e; /* skip space */
  }

  e = l_string_match_ex(map, s, strid, matched_len);
  if (e == 0 || e == l_string_too_short) return 0;
  return e;
}

const l_rune* l_string_trim_head(l_strt s) {
  const l_rune* e = 0;
  while ((e = l_string_match(l_string_space_map(), s)) != 0 && e != l_string_too_short) {
    s.start = e; /* skip space */
  }
  return s.start;
}

const l_rune* l_string_skip_space_and_match_sub(l_strt sub, l_strt s) {
  const l_rune* e = 0;
  while ((e = l_string_match(l_string_space_map(), s)) != 0 && e != l_string_too_short) {
    s.start = e; /* skip space */
  }

  if (s.end - s.start < sub.end - sub.start) return 0;
  while (sub.start < sub.end) {
    if (*sub.start++ != *s.start++) return 0;
  }
  return s.start;
}

void l_string_test() {
  const l_strn methods[] = {l_literal_strn("GET"), l_literal_strn("HEAD"), l_literal_strn("POST")};
  const l_strn orderedchice[] = {l_literal_strn("mankind"), l_literal_strn("man"), l_literal_strn("got"),
      l_literal_strn("gotten"), l_literal_strn("pick"), l_literal_strn("tick"), l_literal_strn("cook")};
  const l_rune subject[] = "gEtoHEAdopOStogeoend";
  const l_rune* s = 0;
  l_stringmap map;
  l_string str = l_create_string(8);

  l_string_format_1(&str, "%f", lf(3.1415926));
  l_logd_1("%f", lf(3.1415926));
  printf("[D] %.80f\n", 3.1415926);
  l_assert(l_string_equal(&str, l_literal_strt("3.14159260000000006840537025709636509418487548828125")));

  l_assert(l_right_most_bit(0x0000) == 0x0000);
  l_assert(l_right_most_bit(0x0001) == 0x0001);
  l_assert(l_right_most_bit(0x0010) == 0x0010);
  l_assert(l_right_most_bit(0x0011) == 0x0001);
  l_assert(l_right_most_bit(0x0100) == 0x0100);
  l_assert(l_right_most_bit(0x0101) == 0x0001);
  l_assert(l_right_most_bit(0x0110) == 0x0010);
  l_assert(l_right_most_bit(0x0111) == 0x0001);
  l_assert(l_right_most_bit(0x1000) == 0x1000);
  l_assert(l_right_most_bit(0x1001) == 0x0001);
  l_assert(l_right_most_bit(0x1010) == 0x0010);
  l_assert(l_right_most_bit(0x1011) == 0x0001);
  l_assert(l_right_most_bit(0x1100) == 0x0100);
  l_assert(l_right_most_bit(0x1101) == 0x0001);
  l_assert(l_right_most_bit(0x1110) == 0x0010);
  l_assert(l_right_most_bit(0x1111) == 0x0001);

  map = l_string_new_map(8, methods, 3, false);
  s = l_string_match(&map, l_strt_l(subject, 3));
  l_logd_1("get - %s", ls(s));
  s = l_string_match(&map, l_strt_l(subject+1, 3));
  l_assert(s == 0);
  s = l_string_match(&map, l_strt_l(subject+4, 3));
  l_assert(s == l_string_too_short);
  s = l_string_match(&map, l_strt_l(subject+4, 4));
  l_logd_1("head - %s", ls(s));
  s = l_string_match(&map, l_strt_l(subject+9, 3));
  l_assert(s == l_string_too_short);
  s = l_string_match(&map, l_strt_l(subject+9, 5));
  l_logd_1("post - %s", ls(s));
  s = l_string_match(&map, l_strt_l(subject+14, 3));
  l_assert(s == 0);

  l_string_set_map(&map, orderedchice, 7, true);
  s = l_string_match(&map, l_literal_strt("mankind"));
  l_assert(*s == 0);
  s = l_string_match(&map, l_literal_strt("mankin"));
  l_assert(*s == 'k');
  s = l_string_match(&map, l_literal_strt("gotten"));
  l_assert(*s == 't');
  s = l_string_match(&map, l_literal_strt("pon"));
  l_assert(s == 0);
  s = l_string_match(&map, l_literal_strt("con"));
  l_assert(s == 0);
  s = l_string_match(&map, l_literal_strt("tiok"));
  l_assert(s == 0);
  s = l_string_match(&map, l_literal_strt("tick"));
  l_assert(*s == 0);

  l_string_free_map(&map);
  l_string_free(&str);

  l_assert(l_check_is_dec_digit('0') && l_check_is_hex_digit('0'));
  l_assert(l_check_is_dec_digit('1') && l_check_is_hex_digit('1'));
  l_assert(l_check_is_dec_digit('2') && l_check_is_hex_digit('2'));
  l_assert(l_check_is_dec_digit('3') && l_check_is_hex_digit('3'));
  l_assert(l_check_is_dec_digit('4') && l_check_is_hex_digit('4'));
  l_assert(l_check_is_dec_digit('5') && l_check_is_hex_digit('5'));
  l_assert(l_check_is_dec_digit('6') && l_check_is_hex_digit('6'));
  l_assert(l_check_is_dec_digit('7') && l_check_is_hex_digit('7'));
  l_assert(l_check_is_dec_digit('8') && l_check_is_hex_digit('8'));
  l_assert(l_check_is_dec_digit('9') && l_check_is_hex_digit('9'));

  l_assert(l_check_is_letter('a') && l_check_is_lower_letter('a') && l_check_is_hex_digit('a'));
  l_assert(l_check_is_letter('b') && l_check_is_lower_letter('b') && l_check_is_hex_digit('b'));
  l_assert(l_check_is_letter('c') && l_check_is_lower_letter('c') && l_check_is_hex_digit('c'));
  l_assert(l_check_is_letter('d') && l_check_is_lower_letter('d') && l_check_is_hex_digit('d'));
  l_assert(l_check_is_letter('e') && l_check_is_lower_letter('e') && l_check_is_hex_digit('e'));
  l_assert(l_check_is_letter('f') && l_check_is_lower_letter('f') && l_check_is_hex_digit('f'));
  l_assert(l_check_is_letter('g') && l_check_is_lower_letter('g'));
  l_assert(l_check_is_letter('h') && l_check_is_lower_letter('h'));
  l_assert(l_check_is_letter('i') && l_check_is_lower_letter('i'));
  l_assert(l_check_is_letter('j') && l_check_is_lower_letter('j'));
  l_assert(l_check_is_letter('k') && l_check_is_lower_letter('k'));
  l_assert(l_check_is_letter('l') && l_check_is_lower_letter('l'));
  l_assert(l_check_is_letter('m') && l_check_is_lower_letter('m'));
  l_assert(l_check_is_letter('n') && l_check_is_lower_letter('n'));
  l_assert(l_check_is_letter('o') && l_check_is_lower_letter('o'));
  l_assert(l_check_is_letter('p') && l_check_is_lower_letter('p'));
  l_assert(l_check_is_letter('q') && l_check_is_lower_letter('q'));
  l_assert(l_check_is_letter('r') && l_check_is_lower_letter('r'));
  l_assert(l_check_is_letter('s') && l_check_is_lower_letter('s'));
  l_assert(l_check_is_letter('t') && l_check_is_lower_letter('t'));
  l_assert(l_check_is_letter('u') && l_check_is_lower_letter('u'));
  l_assert(l_check_is_letter('v') && l_check_is_lower_letter('v'));
  l_assert(l_check_is_letter('w') && l_check_is_lower_letter('w'));
  l_assert(l_check_is_letter('x') && l_check_is_lower_letter('x'));
  l_assert(l_check_is_letter('y') && l_check_is_lower_letter('y'));
  l_assert(l_check_is_letter('z') && l_check_is_lower_letter('z'));

  l_assert(l_check_is_letter('A') && l_check_is_upper_letter('A') && l_check_is_hex_digit('A'));
  l_assert(l_check_is_letter('B') && l_check_is_upper_letter('B') && l_check_is_hex_digit('B'));
  l_assert(l_check_is_letter('C') && l_check_is_upper_letter('C') && l_check_is_hex_digit('C'));
  l_assert(l_check_is_letter('D') && l_check_is_upper_letter('D') && l_check_is_hex_digit('D'));
  l_assert(l_check_is_letter('E') && l_check_is_upper_letter('E') && l_check_is_hex_digit('E'));
  l_assert(l_check_is_letter('F') && l_check_is_upper_letter('F') && l_check_is_hex_digit('F'));
  l_assert(l_check_is_letter('G') && l_check_is_upper_letter('G'));
  l_assert(l_check_is_letter('H') && l_check_is_upper_letter('H'));
  l_assert(l_check_is_letter('I') && l_check_is_upper_letter('I'));
  l_assert(l_check_is_letter('J') && l_check_is_upper_letter('J'));
  l_assert(l_check_is_letter('K') && l_check_is_upper_letter('K'));
  l_assert(l_check_is_letter('L') && l_check_is_upper_letter('L'));
  l_assert(l_check_is_letter('M') && l_check_is_upper_letter('M'));
  l_assert(l_check_is_letter('N') && l_check_is_upper_letter('N'));
  l_assert(l_check_is_letter('O') && l_check_is_upper_letter('O'));
  l_assert(l_check_is_letter('P') && l_check_is_upper_letter('P'));
  l_assert(l_check_is_letter('Q') && l_check_is_upper_letter('Q'));
  l_assert(l_check_is_letter('R') && l_check_is_upper_letter('R'));
  l_assert(l_check_is_letter('S') && l_check_is_upper_letter('S'));
  l_assert(l_check_is_letter('T') && l_check_is_upper_letter('T'));
  l_assert(l_check_is_letter('U') && l_check_is_upper_letter('U'));
  l_assert(l_check_is_letter('V') && l_check_is_upper_letter('V'));
  l_assert(l_check_is_letter('W') && l_check_is_upper_letter('W'));
  l_assert(l_check_is_letter('X') && l_check_is_upper_letter('X'));
  l_assert(l_check_is_letter('Y') && l_check_is_upper_letter('Y'));
  l_assert(l_check_is_letter('Z') && l_check_is_upper_letter('Z'));

  l_assert(l_check_is_alphanum_underscore('_'));
  l_assert(l_check_is_alphanum_underscore_hyphen('-'));
}

