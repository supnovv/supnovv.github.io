#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include "thatcore.h"

int l_strt_contain(l_strt s, int ch) {
  while (s.len-- > 0) {
    if (s.start[s.len] == ch) return true;
  }
  return false;
}

int l_strt_equal(l_strt lhs, l_strt rhs) {
  if (lhs.len != rhs.len) return false;
  while (lhs.len-- > 0) {
    if (lhs.start[lhs.len] != rhs.start[lhs.len]) {
      return false;
    }
  }
  return true;
}

static l_byte* l_strbuf_cstr(l_strbuf* self) {
  return (l_byte*)(self + 1);
}

static l_int l_strbuf_capacity(l_strbuf* self) {
  return self->bsize - sizeof(l_strbuf);
}

static int l_strbuf_ensure_capacity(l_strbuf** self, l_int size) {
  l_int limit = (*self)->limit;
  if (limit && size > limit) return false;
  *self = (l_strbuf*)l_thread_ensure_bfsize(&(*self)->node, sizeof(l_strbuf) + size);
  return true;
}

static int l_strbuf_ensure_remain(l_strbuf** self, l_int remainsize) {
  return l_strbuf_ensure_capacity(self, (*self)->size + remainsize);
}

static l_strbuf* l_strbuf_init(l_thread* thread, l_int initsize, l_int maxlimit) {
  l_strbuf* p = 0;
  if (initsize < (l_int)sizeof(l_strbuf)) initsize = sizeof(l_strbuf);
  if (thread) p = (l_strbuf*)l_thread_alloc_buffer(thread, sizeof(l_strbuf) + initsize);
  else p = (l_strbuf*)l_raw_malloc(sizeof(l_strbuf) + initsize);
  *(l_strbuf_cstr(p)) = 0; /* zero terminated */
  p->size = 0;
  p->limit = (maxlimit < 0) ? 0 : maxlimit;
  return p;
}

static void l_strbuf_free(l_thread* thread, l_strbuf* buffer) {
  if (!buffer) return;
  if (!thread) thread = l_thread_self();
  l_thread_free_buffer(thread, &buffer->node);
}

l_string l_thread_create_limited_string(l_thread* thread, l_int initsize, l_int maxlimit) {
  if (!thread) thread = l_thread_self();
  return (l_string){l_strbuf_init(thread, initsize+1, maxlimit)};
}

l_string l_thread_create_limited_string_from(l_thread* thread, l_strt from, l_int maxlimit) {
  l_string s = l_thread_create_limited_string(thread, from.len, maxlimit);
  if (from.start && from.len > 0) {
    l_strbuf* b = s.b;
    l_copy_l(from.start, from.len, l_strbuf_cstr(b));
    b->size = from.len;
    *(l_strbuf_cstr(b) + from.len) = 0;
  }
  return s;
}

l_string l_thread_create_string(l_thread* thread, l_int initsize) {
  return l_thread_create_limited_string(thread, initsize, 0);
}

l_string l_thread_create_string_from(l_thread* thread, l_strt from) {
  return l_thread_create_limited_string_from(thread, from, 0);
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

void l_thread_string_free(l_thread* thread, l_string* self) {
  if (!self->b) return;
  l_strbuf_free(thread, self->b);
  self->b = 0;
}

void l_string_free(l_string* self) {
  l_thread_string_free(0, self);
}

void l_string_clear(l_string* self) {
  l_strbuf* p = self->b;
  *l_strbuf_cstr(p) = 0;
  p->size = 0;
}

void l_string_set(l_string* self, l_strt s) {
  if (!s.start || s.len <= 0) return;
  l_strbuf_ensure_capacity(&self->b, s.len+1);
  l_copy_l(s.start, s.len, l_strbuf_cstr(self->b));
  self->b->size = s.len;
  *(l_strbuf_cstr(self->b) + s.len) = 0;
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

static void l_write_log_to_file(l_string* self) {
  l_strbuf* b = self->b;
  if (b->size > 0) {
    l_thread* thread = (l_thread*)((l_byte*)self - offsetof(l_thread, log));
    l_write_file(&thread->logfile, l_strbuf_cstr(b), b->size);
  }
  *(l_strbuf_cstr(b)) = 0;
  b->size = 0;
}

int l_string_append(l_string* self, l_strt s) {
  const l_rune* end  = 0;
  l_rune *bstr, *dest;
  if (!l_strbuf_ensure_remain(&self->b, s.len+1)) return false;
  bstr = l_strbuf_cstr(self->b);
  dest = bstr + self->b->size;
  end = s.start + s.len;
  while (s.start < end) {
    *dest++ = *s.start++;
  }
  *dest = 0;
  self->b->size += dest - bstr;
  return true;
}

static void l_string_format_out(l_string* self, l_strt s) {
  if (self->b->flags & L_STRING_PRINT_LOG) {
    l_strbuf* b = self->b;
    l_int cap = l_strbuf_capacity(b);
    l_rune* bstr = l_strbuf_cstr(b);
    l_rune* dest = 0;
    const l_rune* end = 0;

    if (cap - b->size < s.len + 1) { /* 1 is for zero-terminated char */
      l_write_log_to_file(self);
      if (cap - b->size < s.len + 1) {
        l_loge_2("cap %d len %d", ld(cap), ld(s.len));
        return;
      }
    }

    dest = bstr + b->size;
    end = s.start + s.len;
    while (s.start < end) {
      *dest++ = *s.start++;
    }

    *dest = 0;
    b->size += dest - bstr;

  } else {
    l_string_append(self, s);
  }
}

static void l_string_format_out_reverse(l_string* self, l_strt s) {
  l_strbuf* b = self->b;
  l_rune* bstr = 0;
  l_rune* dest = 0;
  const l_rune* end = 0;

  if (self->b->flags & L_STRING_PRINT_LOG) {
    l_int cap = l_strbuf_capacity(b);
    if (cap - b->size < s.len + 1) { /* 1 is for zero-terminated char */
      l_write_log_to_file(self);
      if (cap - b->size < s.len + 1) {
        l_loge_2("cap %d len %d", ld(cap), ld(s.len));
        return;
      }
    }
  } else {
    if (!l_strbuf_ensure_remain(&self->b, s.len + 1)) {
      l_loge_1("len %d", ld(s.len));
      return;
    }
  }

  b = self->b; /* self->b may change, re-get */
  bstr = l_strbuf_cstr(b);
  dest = bstr + b->size;
  end = s.start + s.len;

  while (end-- > s.start) {
    *dest++ = *end;
  }

  *dest = 0;
  b->size += dest - bstr;
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
  if (s.len >= width) {
    l_string_format_out(self, s);
  } else {
    l_rune a[128];
    l_copy_l(s.start, s.len, a);
    l_string_format_fill_and_out(self, a, a + s.len, flags);
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

static const l_rune* l_hex_runes[] = {
  l_rstr("0123456789abcdef"), l_rstr("0123456789ABCDEF")
};

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

  if (precise == 0) precise = 80;

  while (f > 0 && precise-- > 0) {
    ipart = l_cast(l_ulong, f * 10);
    *p++ = l_cast(l_rune, ipart + '0');
    f -= ipart;
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

static const l_rune* l_string_format_a_value(l_string* self, const l_rune* start, const l_rune* end, l_value a) {
  l_umedit flags = 0; /* start pointer to '%' and next rune is not a '%' */
  const l_rune* cur = start;
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

    case 'w': case 'W':
      l_string_format_string(self, *((l_strt*)a.p), flags);
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
  log = &thread->log;
  log->b->flags |= L_STRING_PRINT_LOG;
  l_string_format_out(log, l_strt_c(l_rstr(tag) + 2));

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
}

#define L_BLANK_MAX_LEN (3)
#define L_NUM_OF_SPACES (23)
#define L_NUM_OF_NEWLINES (6)
#define L_NUM_OF_BLANKS (29)

static const l_rune* l_blanks[] = {
  l_rstr("\x09"), /* \t */
  l_rstr("\x0b"), /* \v */
  l_rstr("\x0c"), /* \f */
  /* Zs 'Separator, Space' Category - www.fileformat.info/info/unicode/category/Zs/list.htm */
  l_rstr("\x20"), /* 0x20 space */
  l_rstr("\xc2\xa0"), /* 0xA0 no-break space */
  l_rstr("\xe1\x9a\x80"), /* 0x1680 ogham space mark */
  l_rstr("\xe2\x80\xaf"), /* 0x202F narrow no-break space */
  l_rstr("\xe2\x81\x9f"), /* 0x205F medium mathematical space */
  l_rstr("\xe3\x80\x80"), /* 0x3000 ideographic space (chinese blank character) */
  l_rstr("\xe2\x80\x80"), /* 0x2000 en quad */
  l_rstr("\xe2\x80\x81"), /* 0x2001 em quad */
  l_rstr("\xe2\x80\x82"), /* 0x2002 en space */
  l_rstr("\xe2\x80\x83"), /* 0x2003 em space */
  l_rstr("\xe2\x80\x84"), /* 0x2004 three-per-em space */
  l_rstr("\xe2\x80\x85"), /* 0x2005 four-per-em space */
  l_rstr("\xe2\x80\x86"), /* 0x2006 six-per-em space */
  l_rstr("\xe2\x80\x87"), /* 0x2007 figure space */
  l_rstr("\xe2\x80\x88"), /* 0x2008 punctuation space */
  l_rstr("\xe2\x80\x89"), /* 0x2009 thin space */
  l_rstr("\xe2\x80\x8a"), /* 0x200A hair space */
  /* byte order marks */
  l_rstr("\xfe\xff"),
  l_rstr("\xff\xfe"),
  l_rstr("\xef\xbb\xbf"),
  /* new lines */
  l_rstr("\x0a\x0d"), /* \n\r */
  l_rstr("\x0d\x0a"), /* \r\n */
  l_rstr("\x0a"), /* \r */
  l_rstr("\x0d"), /* \n */
  l_rstr("\xe2\x80\xa8"), /* line separator 0x2028 00100000_00101000 -> 1110'0010_10'000000_10'101000 (0xE280A8) */
  l_rstr("\xe2\x80\xa9")  /* paragraph separator 0x2029 00100000_00101001 */
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

void l_string_set_map(l_stringmap* self, const l_rune** str, int numofstr, int casesensitive) {
  int stridx = 0, charidx = 0;
  const l_rune* s = 0;
  l_runetable* t = self->t;
  l_rune ch = 0;

  l_zero_l(self->t, sizeof(l_runetable) * self->size);

  for (; stridx < numofstr; ++stridx) {

    if (stridx + 1 > self->maxnumofstr) {
      l_loge_s("too many strings");
      break;
    }

    s = str[stridx];
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

l_stringmap l_string_new_map(int maxstrlen, const l_rune** str, int numofstr, int casesensitive) {
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

static const l_rune* const l_string_too_short = (const l_rune* const)(l_int)(-1);

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
const l_rune* l_string_match_ex(const l_stringmap* map, const void* s, int len, int* strid, int* mlen) {
  l_umedit prevmatch = 0xFFFFFFFF, curmatch = 0;
  l_umedit matches = 0, headmatch = 0;
  l_runetable* t = 0;
  int i = 0, end = len;
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
    end = map->size;
  }

  while (i < end) {
    t = map->t + i;
    ch = l_rstr(s)[i];

    curmatch = prevmatch & t->a[ch].m;
    if (!curmatch) {
      if (p) {
        headmatch = l_right_most_bit(matches);
        goto MatchSuccess;
      }
      return 0;
    }

    if (t->a[ch].e & curmatch) {
      p = l_rstr(s) + i + 1;
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
  if (mlen) *mlen = p - l_rstr(s);
  return p;
}

const l_rune* l_string_match(const l_stringmap* map, const void* s, int len) {
  return l_string_match_ex(map, s, len, 0, 0);
}

/* match exactly n times, return >=s or ccstringtooshort */
const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, const void* s, int len) {
  const l_rune* e = l_rstr(s);
  int i = 0;

  while (i++ < n) {
    e = l_string_match(map, s, len);
    if (e == 0) {
      return l_rstr(s);
    }
    if (e == l_string_too_short) {
      return e;
    }
    s = e;
    len -= e - l_rstr(s);
  }

  return e;
}

/* return >=s */
const l_rune* l_string_match_repeat(const l_stringmap* map, const void* s, int len, int* lastmatchfailed) {
  const l_rune* e = 0;
  const l_rune* prev = l_rstr(s);

  while ((e = l_string_match(map, s, len)) != 0 && e != l_string_too_short) {
    prev = e;
    s = e;
    len -= e - l_rstr(s);
  }

  if (e == 0) {
    if (lastmatchfailed) *lastmatchfailed = 1;
  } else {
    if (lastmatchfailed) *lastmatchfailed = 0;
  }

  return prev;
}

/* return 0 - too short to match, >=s - match success, n is the length */
const l_rune* l_string_match_until(const l_stringmap* map, const void* s, int len, int* n) {
  const l_rune* e = 0;
  const l_rune* cur = l_rstr(s);

  while ((e = l_string_match(map, cur, len)) == 0) { /* continue only doesn't match */
    ++cur;
    --len;
  }

  if (e == l_string_too_short) {
    if (n) *n = cur - l_rstr(s);
    return 0;
  }

  if (n) *n = e - cur;
  return e;
}

const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, const void* s, int len, int* n) {
  const l_rune* e = 0;

  /* match space and skip */
  while ((e = l_string_match(l_string_space_map(), s, len)) != 0 && e != l_string_too_short) {
    s = l_rstr(s) + 1;
    --len;
  }

  if (e == l_string_too_short) {
    return 0; /* all chars are spaces, or even last space is not complete */
  }

  /* current char is not a space, try match the string */
  return l_string_match_until(map, s, len, n);
}

/* return 0 - match failed; otherwise success, strid and mlen are set */
const l_rune* l_string_skip_space_and_match(const l_stringmap* map, const void* s, int len, int* strid, int* mlen) {
  const l_rune* e = 0;

  /* match space and skip */
  while ((e = l_string_match(l_string_space_map(), s, len)) != 0 && e != l_string_too_short) {
    s = l_rstr(s) + 1;
    --len;
  }

  if (e == l_string_too_short) {
    return 0; /* all chars are spaces, or even last space is not complete */
  }

  /* current char is not a space, try match the string */
  e = l_string_match_ex(map, s, len, strid, mlen);
  if (e == 0 || e == l_string_too_short) {
    return 0;
  }
  return e;
}

void l_string_test() {
  const l_rune* methods[] = {l_rstr("GET"), l_rstr("HEAD"), l_rstr("POST")};
  const l_rune* orderedchice[] = {l_rstr("mankind"), l_rstr("man"), l_rstr("got"),
      l_rstr("gotten"), l_rstr("pick"), l_rstr("tick"), l_rstr("cook")};
  const l_rune subject[] = "gEtoHEAdopOStogeoend";
  const l_rune* s = 0;
  l_stringmap map;

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
  s = l_string_match(&map, subject, 3);
  l_logd_1("get - %s", ls(s));
  s = l_string_match(&map, subject+1, 3);
  l_assert(s == 0);
  s = l_string_match(&map, subject+4, 3);
  l_assert(s == l_string_too_short);
  s = l_string_match(&map, subject+4, 4);
  l_logd_1("head - %s", ls(s));
  s = l_string_match(&map, subject+9, 3);
  l_assert(s == l_string_too_short);
  s = l_string_match(&map, subject+9, 5);
  l_logd_1("post - %s", ls(s));
  s = l_string_match(&map, subject+14, 3);
  l_assert(s == 0);

  l_string_set_map(&map, orderedchice, 7, true);
  s = l_string_match(&map, "mankind", 7);
  l_assert(*s == 0);
  s = l_string_match(&map, "mankin", 6);
  l_assert(*s == 'k');
  s = l_string_match(&map, "gotten", 6);
  l_assert(*s == 't');
  s = l_string_match(&map, "pon", 3);
  l_assert(s == 0);
  s = l_string_match(&map, "con", 3);
  l_assert(s == 0);
  s = l_string_match(&map, "tiok", 4);
  l_assert(s == 0);
  s = l_string_match(&map, "tick", 4);
  l_assert(*s == 0);

  l_string_free_map(&map);
}

