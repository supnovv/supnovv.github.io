#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <float.h>

#define L_LIBRARY_IMPL
#include "core/string.h"

#define L_COMMON_BUFHEAD l_smplnode node; l_int bsize;
#define l_string_ptr(s) ((l_strbuf*)s->impl)

typedef struct {
  L_COMMON_BUFHEAD
  l_int size;
  l_int limit;
} l_strbuf;

L_PRIVAT l_string l_master_create_string(l_int len, const l_byte* init, l_int maxlimit, l_thread* hint);
L_PRIVAT void* l_master_ensure_bfsize(l_smplnode* buffer, l_int size);
L_PRIVAT void l_master_free_buffer(l_smplnode* buffer, l_thread* hint);
L_PRIVAT void l_master_write_log(l_string* self);
L_PRIVAT void l_master_flush_log();
L_PRIVAT l_string* l_master_start_log(const l_byte* tag);

L_PRIVAT void
l_string_setEnd(l_string* self, l_int len)
{
  l_string_ptr(self)->size = len;
  *(l_string_cstr(self) + len) = 0;
}

L_EXTERN l_string
l_string_empty()
{
  return l_string_create(0);
}

L_EXTERN l_string
l_string_create(l_int initsize)
{
  return l_string_createEx(initsize, 0, 0);
}

L_EXTERN l_string
l_string_createFrom(l_strt from)
{
  return l_string_createFromEx(from, 0, 0);
}

L_EXTERN l_string
l_string_createEx(l_int initsize, l_int maxlimit, l_thread* hint)
{
  return l_master_create_string(initsize, 0, maxlimit, hint);
}

L_EXTERN l_string
l_string_createFromEx(l_strt from, l_int maxlimit, l_thread* hint)
{
  return l_master_create_string(from.end - from.start, from.start, maxlimit, hint);
}

L_EXTERN void
l_string_free(l_string* self, l_thread* hint)
{
  if (self->impl) {
    l_master_free_buffer(&(l_string_ptr(self)->node), hint);
    self->impl = 0;
  }
}

L_EXTERN void
l_string_clear(l_string* self)
{
  *l_string_cstr(self) = 0;
  l_string_ptr(self)->size = 0;
}

L_EXTERN l_byte*
l_string_cstr(l_string* self)
{
  return (l_byte*)(l_string_ptr(self) + 1);
}

L_EXTERN l_int
l_string_capacity(l_string* self)
{
  return l_string_ptr(self)->bsize - sizeof(l_strbuf) - 1;
}

L_EXTERN l_int
l_string_remain(l_string* self)
{
  return l_string_capacity(self) - l_string_size(self);
}

L_EXTERN l_int
l_string_size(l_string* self)
{
  return l_string_ptr(self)->size;
}

L_EXTERN l_int
l_string_limit(l_string* self)
{
  l_int limit = l_string_ptr(self)->limit;
  return limit >= 0 ? limit : -limit;
}

L_EXTERN l_byte*
l_string_start(l_string* self)
{
  return l_string_cstr(self);
}

L_EXTERN l_byte*
l_string_end(l_string* self)
{
  return l_string_cstr(self) + l_string_size(self);
}

L_EXTERN l_strt
l_string_strt(l_string* self)
{
  return l_strt_n(l_string_start(self), l_string_ptr(self)->size);
}

L_EXTERN l_strn
l_string_strn(l_string* self)
{
  return l_strn_n(l_string_start(self), l_string_ptr(self)->size);
}

L_EXTERN int
l_string_equal(l_string* self, l_strt s)
{
  return l_strt_equal(l_string_strt(self), s);
}

L_EXTERN int
l_string_isEmpty(l_string* self)
{
  return l_string_ptr(self)->size == 0;
}

L_EXTERN int
l_string_ntEmpty(l_string* self)
{
  return l_string_ptr(self)->size != 0;
}

L_EXTERN int
l_string_ensureCapacity(l_string* self, l_int size)
{
  void* buffer = 0;
  l_int limit = l_string_limit(self);
  if (limit > 0 && size > limit) {
    return false;
  }
  if (!(buffer = l_master_ensure_bfsize(&(l_string_ptr(self)->node), sizeof(l_strbuf) + size + 1))) {
    return false;
  }
  self->impl = buffer;
  return true;
}

L_EXTERN int
l_string_ensureRemain(l_string* self, l_int remainsize)
{
  return l_string_ensureCapacity(self, l_string_size(self) + remainsize);
}

L_EXTERN int
l_string_set(l_string* self, l_strt s)
{
  return l_string_setLen(self, s.start, s.end - s.start);
}

L_EXTERN int
l_string_setLen(l_string* self, const void* s, l_int len)
{
  if (!l_string_ensureCapacity(self, len)) {
    return false;
  }
  if (!l_copy_n(s, len, l_string_cstr(self))) {
    return false;
  }
  l_string_setEnd(self, len);
  return true;
}

L_EXTERN int
l_string_append(l_string* self, l_strt s)
{
  return l_string_appendLen(self, s.start, s.end - s.start);
}

L_EXTERN int
l_string_appendReversed(l_string* self, l_strt s)
{
  l_int len = s.end - s.start;
  l_byte* p = 0;

  if (len <= 0)
    return false;

  if (!l_string_ensureRemain(self, len))
    return false;

  p = l_string_end(self);
  while (s.start < s.end--) {
    *p++ = *s.end;
  }

  *p = 0;
  l_string_ptr(self)->size += len;
  return true;
}

L_EXTERN int
l_string_appendLen(l_string* self, const void* s, l_int len)
{
  if (!l_string_ensureRemain(self, len)) {
    return false;
  }
  if (!l_copy_n(s, len, l_string_end(self))) {
    return false;
  }
  l_string_setEnd(self, l_string_size(self) + len);
  return true;
}

L_EXTERN l_int
l_string_appendPossible(l_string* self, l_strt s)
{
  return l_string_appendLenPossible(self, s.start, s.end - s.start);
}

L_EXTERN l_int
l_string_appendReversedPossible(l_string* self, l_strt s)
{
  l_int remain = l_string_remain(self);
  l_int len = s.end - s.start;
  l_byte* p = 0;

  if (remain <= 0 || len <= 0 || !s.start)
    return 0;

  if (remain < len) {
    len = remain;
    s.start = s.end - remain;
  }

  p = l_string_end(self);
  while (s.start < s.end--) {
    *p++ = *s.end;
  }

  *p = 0;
  l_string_ptr(self)->size += len;
  return len;
}

L_EXTERN l_int
l_string_appendLenPossible(l_string* self, const void* s, l_int len)
{
  l_int remain = l_string_remain(self);
  if (remain <= 0 || len <= 0)
    return 0;

  if (remain < len) len = remain;

  if (!l_copy_n(s, len, l_string_end(self)))
    return 0;

  l_string_setEnd(self, l_string_size(self) + len);
  return len;
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

static void
l_string_format_out(l_string* self, l_strt s)
{
  l_int len = s.end - s.start;
  if (len <= 0)
    return;

  if (l_string_ptr(self)->limit >=0) {
    l_string_append(self, s);
    return;
  }

  for (; ;) {
    s.start += l_string_appendPossible(self, s);

    if (l_string_remain(self) > 0)
      break;

    l_master_write_log(self);
  }
}

static void
l_string_format_out_reverse(l_string* self, l_strt s)
{
  l_int len = s.end - s.start;
  if (len <= 0) return;

  if (l_string_ptr(self)->limit >= 0) {
    l_string_appendReversed(self, s);
    return;
  }

  for (; ;) {
    s.end -= l_string_appendReversedPossible(self, s);

    if (l_string_remain(self) > 0)
      break;

    l_master_write_log(self);
  }
}

static void
l_string_format_fill_and_out(l_string* self, l_byte* a, l_byte* p, l_umedit flags)
{
  l_byte fill = l_cast(l_byte, flags & 0xff);
  int width = ((flags & 0x7f00) >> 8);
  int reverse = (flags & L_FORMAT_REVERSE) != 0;
  int left = (flags & L_FORMAT_LEFT) != 0;

  if (a >= p) return;

  if (width > p - a) {
    if (!fill) fill = ' ';

    if (reverse == left) {
      l_byte* end = a + width;

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
    l_string_format_out_reverse(self, l_strt_from(a, p));
  } else {
    l_string_format_out(self, l_strt_from(a, p));
  }
}

static void
l_string_format_string(l_string* self, l_strt s, l_umedit flags)
{
  int width = ((flags & 0x7f00) >> 8);
  l_int len = s.end - s.start;
  if (len >= width) {
    l_string_format_out(self, s);
  } else {
    l_byte a[128];
    memcpy(a, s.start, len);
    l_string_format_fill_and_out(self, a, a + len, flags);
  }
}

static void
l_string_format_char(l_string* self, l_ulong a, l_umedit flags)
{
  l_byte ch = l_cast(l_byte, a&0xff);
  if ((flags & L_FORMAT_UPPER) && ch >= 'a' && ch <= 'z') {
    ch -= 32;
  }
  l_string_format_string(self, l_strt_n(&ch, 1), flags);
}

static void
l_string_format_bool(l_string* self, l_ulong a, l_umedit flags)
{
  if (a) {
    l_string_format_string(self, (flags & L_FORMAT_UPPER) ? l_strt_literal("TRUE") : l_strt_literal("true"), flags);
  } else {
    l_string_format_string(self, (flags & L_FORMAT_UPPER) ? l_strt_literal("FALSE") : l_strt_literal("false"), flags);
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
L_GLOBAL const l_byte l_char_class_table[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, 0x80,0x80,0x80,0x80,0x80,0x20,0x80,0x80, 0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d,0x3d, 0x3d,0x3d,0x80,0x80,0x80,0x80,0x80,0x80, /* (20) - (3d) 0 ~ 9 */
  0x80,0x3e,0x3e,0x3e,0x3e,0x3e,0x3e,0x3a, 0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a, 0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a, 0x3a,0x3a,0x3a,0x80,0x80,0x80,0x80,0x30, /* (3e,3a) A ~ Z (30) _ */
  0x80,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3b, 0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b, 0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b, 0x3b,0x3b,0x3b,0x80,0x80,0x80,0x80,0x00, /* (3f,3b) a ~ z */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

L_GLOBAL const l_byte* l_hex_digits[] = {
  l_cstr("0123456789abcdef"), l_cstr("0123456789ABCDEF")
};

L_EXTERN l_int
l_string_parseDec(l_strt s)
{
  l_int times = 1;
  l_int value = 0;
  const l_byte* start = 0;
  const l_byte* end = 0;
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

L_EXTERN l_int
l_string_parseHex(l_strt s)
{
  l_int times = 1;
  l_int value = 0;
  const l_byte* start = 0;
  const l_byte* end = 0;
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


static void
l_string_format_ulong(l_string* self, l_ulong n, l_umedit flags)
{
  /* 64-bit unsigned int max value 18446744073709552046 (20 chars) */
  l_byte a[127];
  l_byte basechar = 0;
  l_byte* p = a;
  const l_byte* hex = 0;
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
    hex = l_hex_digits[(flags & L_FORMAT_UPPER) != 0];
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

static void
l_string_format_long(l_string* self, l_long a, l_umedit flags)
{
  l_ulong n = a;
  if (a < 0) {
    n = (-a);
    flags |= L_FORMAT_NEGSIGN;
  }
  l_string_format_ulong(self, n, flags);
}

L_EXTERN l_byte*
l_string_print_ulong(l_ulong n, l_byte* p)
{
  l_byte a[80];
  l_byte* s = a;

  *s++ = (n % 10) + '0';

  while ((n /= 10)) {
    *s++ = (n % 10) + '0';
  }

  while (s-- > a) {
    *p++ = *s;
  }

  return p;
}

static l_byte*
l_string_print_fraction(double f, l_byte* p, int precise)
{
  l_ulong ipart = 0;

  if (f < DBL_EPSILON) {
    *p++ = '0';
    return p;
  }

  if (precise == 0) precise = 80;
  while (f >= DBL_EPSILON && precise-- > 0) {
    ipart = l_cast(l_ulong, f * 10);
    *p++ = l_cast(l_byte, ipart + '0');
    f = f * 10 - ipart;
  }

  return p;
}

static void
l_string_format_float(l_string* self, l_value v, l_umedit flags)
{
  l_byte a[144];
  l_byte sign = 0;
  l_byte* p = a;
  l_byte* dot = 0;
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

static const l_byte*
l_string_format_a_value(l_string* self, const l_byte* start, const l_byte* end, l_value a)
{
  l_umedit flags = 0; /* start pointer to '%' and next char is not a '%' */
  const l_byte* cur = start;
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
      flags |= *cur; /* fill char */
      continue;

    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
        l_umedit width = *cur - '0';
        l_byte ch = 0;
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
          l_string_format_string(self, l_strn_strt((l_strn*)a.p), flags);
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
      l_string_format_bool(self, (int)a.u, flags);
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

  l_string_format_out(self, l_strt_from(start, end));
  return 0;
}

static int
l_string_format_v_impl(l_string* self, const l_byte* fmt, int n, va_list vl)
{
  int nfmts = 0;
  int nargs = n;
  const l_byte* cur = fmt;
  const l_byte* end = cur + strlen((char*)fmt);

  if (nargs <= 0 || nargs > 9) {
    l_string_format_out(self, l_strt_c(fmt));
    return 0;
  }

  while (cur < end) {
    if (*cur != '%') {
      ++cur;
      continue;
    }

    l_string_format_out(self, l_strt_from(fmt, cur));

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
    l_string_format_out(self, l_strt_from(fmt, end));
  }

  return nfmts;
}

L_EXTERN int
l_string_format_n_impl(l_string* self, const void* fmt, int n, l_value* a)
{
  int nfmts = 0;
  const l_byte* cur = 0;
  const l_byte* end = 0;
  const l_byte* beg = 0;

  if (!fmt) return 0;

  if (n <= 0 || !a) {
    l_string_format_out(self, l_strt_c(fmt));
    return 0;
  }

  cur = l_cstr(fmt);
  end = cur + strlen((char*)fmt);
  beg = cur;

  while (cur < end) {
    if (*cur != '%') {
      ++cur;
      continue;
    }

    l_string_format_out(self, l_strt_from(beg, cur));

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
    l_string_format_out(self, l_strt_from(beg, cur));
  }

  return nfmts;
}

static int
l_string_format_impl(l_string* self, const void* fmt, int n, ...)
{
  int nfmts = 0;
  va_list vl;

  if (!fmt) return 0;

  va_start(vl, n);
  nfmts = l_string_format_v_impl(self, l_cstr(fmt), n, vl);
  va_end(vl);

  return nfmts;
}

L_PRIVAT void
l_logger_func_impl(const void* tag, const void* fmt, ...)
{
  int level = l_cstr(tag)[0] - '0';
  int nargs = l_cstr(tag)[1];
  l_string* log = 0;
  va_list vl;

  if (!fmt || level < 0 || level > l_logger_getLevel()) {
    return;
  }

  log = l_master_start_log(l_cstr(tag) + 2);

  if (nargs == 'n') {
    va_start(vl, fmt);
    nargs = va_arg(vl, l_int);
    l_string_format_n_impl(log, fmt, nargs, va_arg(vl, l_value*));
    va_end(vl);
  } else {
    va_start(vl, fmt);
    l_string_format_v_impl(log, l_cstr(fmt), nargs-'0', vl);
    va_end(vl);
  }

  l_string_format_out(log, l_strt_n(L_NEWLINE, L_NL_SIZE));
}

L_EXTERN void
l_logger_flush()
{
  l_master_flush_log();
}

L_EXTERN int
l_string_format_1(l_string* self, const void* fmt, l_value a)
{
  return l_string_format_impl(self, fmt, 1, a);
}

L_EXTERN int
l_string_format_2(l_string* self, const void* fmt, l_value a, l_value b)
{
  return l_string_format_impl(self, fmt, 2, a, b);
}

L_EXTERN int
l_string_format_3(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c)
{
  return l_string_format_impl(self, fmt, 3, a, b, c);
}

L_EXTERN int
l_string_format_4(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d)
{
  return l_string_format_impl(self, fmt, 4, a, b, c, d);
}

L_EXTERN int
l_string_format_5(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e)
{
  return l_string_format_impl(self, fmt, 5, a, b, c, d, e);
}

L_EXTERN int
l_string_format_6(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f)
{
  return l_string_format_impl(self, fmt, 6, a, b, c, d, e, f);
}

L_EXTERN int
l_string_format_7(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g)
{
  return l_string_format_impl(self, fmt, 7, a, b, c, d, e, f, g);
}

L_EXTERN int
l_string_format_8(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g, l_value h)
{
  return l_string_format_impl(self, fmt, 8, a, b, c, d, e, f, g, h);
}

L_EXTERN int
l_string_format_9(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i)
{
  return l_string_format_impl(self, fmt, 9, a, b, c, d, e, f, g, h, i);
}

L_EXTERN void
l_string_test()
{
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

