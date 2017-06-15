#include <string.h>
#include <stdarg.h>
#include "string.h"

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

static void l_log_print(l_logger* l, const l_rune* s, const l_rune* e) {
  if (l->capacity - l->size <= e - s) {
    l_log_write_to_file(l);
  }
  while (s < e) {
    l->a[l->size++] = *s++;
  }
}

static void l_log_printlstr(l_logger* self, const l_rune* s, int len) {
  l_log_print(self, s, s + len);
}

static void l_log_printcstr(l_logger* self, const l_rune* s) {
  l_log_printlstr(self, s, strlen(l_cast(char*, s)));
}

static void l_log_print_reverse(l_logger* l, const l_rune* s, const l_rune* e) {
  if (l->capacity - l->size <= e - s) {
    l_log_write_to_file(l);
  }
  while (e > s) {
    l->a[l->size++] = *(--e);
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
    l_log_print_reverse(log, a, p);
  } else {
    l_log_print(log, a, p);
  }
}

static void l_log_string(l_logger* log, const l_rune* s, const l_rune* e, l_umedit flags) {
  int width = ((flags & 0x7f00) >> 8);
  int len = e - s;
  if (len >= width) {
    l_log_print(log, s, e);
  } else {
    l_rune a[128];
    l_copy_l(s, len, a);
    l_log_string_fill_and_print(log, a, a + len, flags);
  }
}

static void l_log_lstring(l_logger* log, const l_rune* s, int len, l_umedit flags) {
  l_log_string(log, s, s + len, flags);
}

static void l_log_format_string(l_logger* log, const void* a, l_umedit flags) {
  l_log_lstring(log, l_str(a), strlen((char*)a), flags);
}

static void l_log_format_strfrom(l_logger* log, const void* a, l_umedit flags) {
  const l_strt* s = l_cast(const l_strt*, a);
  l_log_string(log, s->start, s->end, flags);
}

static void l_log_format_char(l_logger* log, l_uinteger a, l_umedit flags) {
  l_rune ch = l_cast(l_rune, a&0xff);
  if ((flags & L_FORMAT_UPPER) && ch >= 'a' && ch <= 'z') {
    ch -= 32;
  }
  l_log_lstring(log, &ch, 1, flags);
}

static void l_log_format_true(l_logger* log, l_uinteger a, l_umedit flags) {
  if (a) {
    l_log_lstring(log, (flags & L_FORMAT_UPPER) ? "TRUE" : "true" , 4, flags);
  } else {
    l_log_lstring(log, (flags & L_FORMAT_UPPER) ? "FALSE" : "false", 5, flags);
  }
}

static const l_rune* l_hex_runes[] = {
  "0123456789abcdef", "0123456789ABCDEF"
};

static void l_log_format_uinteger(l_logger* log, l_uinteger n, l_umedit flags) {
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

static void l_log_format_integer(l_logger* log, l_integer a, l_umedit flags) {
  l_uinteger n = a;
  if (a < 0) {
    n = (-a);
    flags |= L_FORMAT_NEGSIGN;
  }
  l_log_format_uinteger(log, n, flags);
}

static void l_log_print_uinteger(l_uinteger n, l_rune* p) {
  (void)n;
  (void)p;
}

static void l_log_print_fraction(l_uinteger n, l_uinteger intmasks, l_rune* p) {
  (void)n;
  (void)p;
} 

static void l_log_format_float(l_logger* log, l_value v, l_umedit flags) {
  l_rune a[127];
  l_rune* p = a;
  l_rune* point = 0;
  l_uinteger fraction = 0;
  l_uinteger mantissa = 0;
  l_uinteger intmasks = 0;
  int exponent = 0;
  int negative = 0;
  int len = 0;

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
          l_log_print_uinteger(mantissa, p);
          *p++ = '.';
          *p++ = '0';
        } else {
          exponent -= 52;
          l_log_print_uinteger(mantissa, p);
          *p++ = '*';
          *p++ = '2';
          *p++ = '^';
          l_log_print_uinteger(exponent, p);
        }
      } else {
        /* have integer part and fraction part */
        intmasks >>= exponent;
        l_log_print_uinteger((mantissa & intmasks) >> (52 - exponent), p);
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

  level = l_str(tag)[0] - '0';
  numofargs = l_str(tag)[1] - '0';

  if (level < 0 || level > l_log_level) {
    return;
  }

  log = l_thread_logger();
  l_log_printcstr(log, l_str(tag) + 2);
  if (numofargs <= 0) {
    l_log_printcstr(log, fmt);
    return;
  }

  start = l_str(fmt);
  cur = start;

  va_start(vl, fmt);

  while (*cur) {
    if (*cur != '%') {
      ++cur;
      continue;
    }

    l_log_print(log, start, cur);
    start = cur;
    if (fmtcnt >= numofargs) {
      break;
    }

    /**
     * s - const void*  // ?x print as hex, ?w treat as l_strt*
     * f - double
     * u - l_uinteger
     * d - l_integer
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
      l_log_format_uinteger(log, va_arg(vl, l_value).u, flags);
      break;

    case 'd': case 'D':
      ++fmtcnt;
      l_log_format_integer(log, va_arg(vl, l_value).d, flags);
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
      l_log_format_uinteger(log, va_arg(vl, l_value).u, flags);
      break;

    case 'O':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'o':
      flags |= L_FORMAT_OCT;
      ++fmtcnt;
      l_log_format_uinteger(log, va_arg(vl, l_value).u, flags);
      break;

    case 'X': case 'P':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'x': case 'p':
      flags |= L_FORMAT_HEX;
      ++fmtcnt;
      l_log_format_uinteger(log, va_arg(vl, l_value).u, flags);
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
    l_log_print(log, start, cur);
  }
}


#define L_BLANK_MAX_LEN (3)
#define L_NUM_OF_SPACES (23)
#define L_NUM_OF_NEWLINES (6)
#define L_NUM_OF_BLANKS (29)

static const l_rune* l_blanks[] = {
  l_str("\x09"), /* \t */
  l_str("\x0b"), /* \v */
  l_str("\x0c"), /* \f */
  /* Zs 'Separator, Space' Category - www.fileformat.info/info/unicode/category/Zs/list.htm */
  l_str("\x20"), /* 0x20 space */
  l_str("\xc2\xa0"), /* 0xA0 no-break space */
  l_str("\xe1\x9a\x80"), /* 0x1680 ogham space mark */
  l_str("\xe2\x80\xaf"), /* 0x202F narrow no-break space */
  l_str("\xe2\x81\x9f"), /* 0x205F medium mathematical space */
  l_str("\xe3\x80\x80"), /* 0x3000 ideographic space (chinese blank character) */
  l_str("\xe2\x80\x80"), /* 0x2000 en quad */
  l_str("\xe2\x80\x81"), /* 0x2001 em quad */
  l_str("\xe2\x80\x82"), /* 0x2002 en space */
  l_str("\xe2\x80\x83"), /* 0x2003 em space */
  l_str("\xe2\x80\x84"), /* 0x2004 three-per-em space */
  l_str("\xe2\x80\x85"), /* 0x2005 four-per-em space */
  l_str("\xe2\x80\x86"), /* 0x2006 six-per-em space */
  l_str("\xe2\x80\x87"), /* 0x2007 figure space */
  l_str("\xe2\x80\x88"), /* 0x2008 punctuation space */
  l_str("\xe2\x80\x89"), /* 0x2009 thin space */
  l_str("\xe2\x80\x8a"), /* 0x200A hair space */
  /* byte order marks */
  l_str("\xfe\xff"),
  l_str("\xff\xfe"),
  l_str("\xef\xbb\xbf"),
  /* new lines */
  l_str("\x0a\x0d"), /* \n\r */
  l_str("\x0d\x0a"), /* \r\n */
  l_str("\x0a"), /* \r */
  l_str("\x0d"), /* \n */
  l_str("\xe2\x80\xa8"), /* line separator 0x2028 00100000_00101000 -> 1110'0010_10'000000_10'101000 (0xE280A8) */
  l_str("\xe2\x80\xa9")  /* paragraph separator 0x2029 00100000_00101001 */
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

static const l_rune* const l_string_too_short = (const l_rune* const)(l_intptr)(-1);

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
    ch = l_str(s)[i];

    curmatch = prevmatch & t->a[ch].m;
    if (!curmatch) {
      if (p) {
        headmatch = l_right_most_bit(matches);
        goto MatchSuccess;
      }
      return 0;
    }

    if (t->a[ch].e & curmatch) {
      p = l_str(s) + i + 1;
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
  if (mlen) *mlen = p - l_str(s);
  return p;
}

const l_rune* l_string_match(const l_stringmap* map, const void* s, int len) {
  return l_string_match_ex(map, s, len, 0, 0);
}

/* match exactly n times, return >=s or ccstringtooshort */
const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, const void* s, int len) {
  const l_rune* e = l_str(s);
  int i = 0;

  while (i++ < n) {
    e = l_string_match(map, s, len);
    if (e == 0) {
      return l_str(s);
    }
    if (e == l_string_too_short) {
      return e;
    }
    s = e;
    len -= e - l_str(s);
  }

  return e;
}

/* return >=s */
const l_rune* l_string_match_repeat(const l_stringmap* map, const void* s, int len, int* lastmatchfailed) {
  const l_rune* e = 0;
  const l_rune* prev = l_str(s);

  while ((e = l_string_match(map, s, len)) != 0 && e != l_string_too_short) {
    prev = e;
    s = e;
    len -= e - l_str(s);
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
  const l_rune* cur = l_str(s);

  while ((e = l_string_match(map, cur, len)) == 0) { /* continue only doesn't match */
    ++cur;
    --len;
  }

  if (e == l_string_too_short) {
    if (n) *n = cur - l_str(s);
    return 0;
  }

  if (n) *n = e - cur;
  return e;
}

const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, const void* s, int len, int* n) {
  const l_rune* e = 0;

  /* match space and skip */
  while ((e = l_string_match(l_string_space_map(), s, len)) != 0 && e != l_string_too_short) {
    s = l_str(s) + 1;
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
    s = l_str(s) + 1;
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
  const l_rune* methods[] = {l_str("GET"), l_str("HEAD"), l_str("POST")};
  const l_rune* orderedchice[] = {l_str("mankind"), l_str("man"), l_str("got"),
      l_str("gotten"), l_str("pick"), l_str("tick"), l_str("cook")};
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

