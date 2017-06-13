#ifndef l_string_lib_h
#define l_string_lib_h
#include "thatcore.h"

typedef struct {
  void* file;
  l_rune* a;
  int capacity;
  int size;
} l_logger;

typedef union {
  double f;
  l_integer i;
  l_uinteger u;
  const void* s;
} l_value;

void l_log_func_s(const void* s) {
  l_log_func_impl(s, 0, 0);
}

void l_log_func_one(const void* fmt, l_value a) {
  l_log_func_impl(fmt, 1, a);
}

void l_log_func_two(const void* fmt, l_value a, l_value b) {
  l_log_func_impl(fmt, 2, a, b);
}

void l_log_func_three(const void* fmt, l_value a, l_value b, l_value c) {
  l_log_func_impl(fmt, 3, a, b, c);
}

void l_log_func_four(const void* fmt, l_value a, l_value b, l_value c, l_value d) {
  l_log_func_impl(fmt, 4, a, b, c, d);
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

void l_log_string_reverse(l_logger* l, const l_rune* s, const l_rune* e) {
  if (l->capacity - l->size <= e - s) {
    l_log_flush(l);
  }
  while (e > s) {
    l->a[l->size++] = *(--e);
  }
}

void l_log_string(l_logger* log, l_rune* a, l_rune* p, l_umedit flags) {
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

void l_log_printl(l_logger* self, const l_rune* s, int len) {

}

void l_log_printe(l_logger* self, const l_rune* start, const l_rune* end) {
  l_log_printl(self, start, end - start);
}

void l_log_printc(l_logger* self, const void* s) {
  l_log_printl(self, s, strlen(s));
}

void l_log_l(l_logger* log, const rune* s, int len, l_umedit flags) {
  int width = ((flags & 0x7f00) >> 8);
  if (len >= width) {
    l_log_printl(log, s, len);
  } else {
    l_rune a[128];
    l_copyl(s, len, a);
    l_log_print_string(log, a, a + len, flags);
  }
}

void l_log_f(l_logger* log, double a, l_umedit flags) {

}

static const l_rune* l_hex_runes[] = {"0123456789abcdef", "0123456789ABCDEF"};

void l_log_u(l_logger* log, l_uinteger a, l_umedit flags) {
  /* 64-bit unsigned int max value 18446744073709552046 (20 runes) */
  l_rune a[127];
  l_rune basechar = 0;
  l_rune* p = a;
  l_rune* hex = 0;
  l_umedit precise = ((flags & 0x7f0000) >> 16);
  l_umedit base = 0;
  l_umedit sign = 0;

  flags &= L_FORMAT_MASKS;
  base = (flags & L_FORAMT_BSMASKS);

  switch (l_right_most_bit(base)) {

  case 0:
    *p++ = (a % 10) + '0';
    while ((n /= 10)) {
      a[i++] = (n % 10) + '0';
    }
    break;

  case L_FORMAT_HEX:
    hex = l_hex_runes[flags & L_FORMAT_UPPER];
    *p++ = hex[a & 0x0f];
    while ((a >>= 4)) {
      *p++ = hex[a & 0x0f];
    }
    if (!(flags & L_FORMAT_N0B0O0X)) {
      basechar = (flags & L_FORMAT_UPPER) ? 'X' : 'x';
    }
    flags &= (~L_FORMAT_SNMASKS);
    break;

  case L_FORMAT_OCT:
    *p++ = (a & 0x07) + '0';
    while ((a >>= 3)) {
      *p++ = (a & 0x07) + '0';
    }
    if (!(flags & L_FORMAT_N0B0O0X)) {
      basechar = (flags & L_FORMAT_UPPER) ? 'O' : 'o';
    }
    flags &= (~L_FORMAT_SNMASKS);
    break;

  case L_FORMAT_BIN:
    *p++ = (a & 0x01) + '0';
    while ((a >>= 1)) {
      *p++ = (a & 0x01) + '0';
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

  l_log_print_string(log, a, p, flags | L_FORMAT_REVERSE);
}

void l_log_s(l_logger* log, const void* a, l_umedit flags) {
  l_log_l(log, l_str(a), strlen(l_cast(char*)a), flags);
}

void l_log_i(l_logger* log, l_integer a, l_umedit flags) {
  l_uinteger n = a;
  if (a < 0) {
    n = (-a);
    flags |= L_FORMAT_NEGSIGN;
  }
  l_log_u(log, n, flags);
}

void l_log_c(l_logger* log, l_uinteger a, l_umedit flags) {

}

void l_log_t(l_logger* log, l_uinteger a, l_umedit flags) {
  if (a) {
    l_log_l(log, "true", 4, flags);
  } else {
    l_log_l(log, "false", 5, flags);
  }
}

void l_log_w(l_logger* log, const void* a, l_umedit flags) {
  const l_from* s = l_cast(const l_from*, a);
  l_log_l(log, s->start, s->end - s->start, flags);
}

static int l_log_level = 2;

void l_log_func_impl(const char* tag, const void* fmt, int n, ...) {
  int nfmt = 0, level = tag[0] - '0';
  const l_rune* prev = l_str(fmt);
  const l_rune* cur = prev;
  l_logger* log = 0;
  l_umedit flags = 0, width = 0;
  int fmtend = 0, ch = 0;
  va_list vl;

  if (level > l_log_level || !fmt) return 0;

  log = l_service_log();

  if (n <= 0) {
    l_log_cstring(log, fmt);
    return;
  }

  va_start(vl, n);

  while (*cur) {
    if (*cur != '%') {
      ++cur;
      continue;
    }

    l_log_printe(log, prev, cur);
    prev = cur;
    if (nfmt >= n) {
      break;
    }

    /**
     * s - const void*  // ?x print as hex, ?w treat as l_from*
     * f - double
     * u - l_uinteger
     * i - l_integer
     * w - l_from*
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
      /* current is the 2nd digit, skip next extra digits */
      while ((ch = *(cur + 1)) >= '0' && ch <= '9') {
        ++cur;
      }
      /* current char is digit, and next is not */
      if (flags & L_FORMAT_PRECISE) {
        flags |= (width << 16)
      } else {
        flags |= (width << 8);
      }
      goto ParseFormat;
    case 's': case 'S':
      ++nfmt;
      l_log_cstring(log, va_arg(vl, l_value).s, flags);
      break;
    case 'f': case 'F':
      ++nfmt;
      l_log_float(log, va_arg(vl, l_value).f, flags);
      break;
    case 'u': case 'U':
      ++nfmt;
      l_log_uinteger(log, va_arg(vl, l_value).u, flags);
      break;
    case 'i': case 'I':
      ++nfmt;
      l_log_integer(log, va_arg(vl, l_value).i, flags);
      break;
    case 'm': case 'M':
      ++nfmt;
      l_log_sfrom(log, va_arg(vl, l_value).s, flags);
      break;
    case 'c': case 'C':
      ++nfmt;
      l_log_char(log, va_arg(vl, l_value).u, flags);
      break;
    case 't': cast 'T':
      ++nfmt;
      l_log_true(log, va_arg(vl, l_value).u, flags);
      break;
    case 'B':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'b':
      flags |= L_FORMAT_BIN;
      ++nfmt;
      l_log_uinteger(log, va_arg(vl, l_value).u, flags);
      break;
    case 'O':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'o':
      flags |= L_FORMAT_OCT;
      ++nfmt;
      l_log_uinteger(log, va_arg(vl, l_value).u, flags);
      break;
    case 'X': case 'P':
      flags |= L_FORMAT_UPPER;
      /* fallthrough */
    case 'x': case 'p':
      flags |= L_FORMAT_HEX;
      ++nfmt;
      l_log_uinteger(log, va_arg(vl, l_value).u. flags);
      break;
    case 0:
      fmtend = 1;
      break;
    case '%':
      /* find the 2nd '%' */
      if (cur - prev == 1) {
        prev = cur++;
        /* %%abc - continue at 'a' */
      }
      /* %z%abc - continue at '%' */
      /* fallthrough */
    default:
      /* invalid format char, continue to while */
      cointinue;
    }

    if (fmtend) {
      break;
    }

    ++cur;
    prev = cur;
  }

  va_end(vl);

  if (prev < cur) {
    l_log_string(log, prev, cur);
  }
}

typedef struct {
  l_umedit m; /* string match this rune */
  l_umedit e; /* string ended with this rune */
} l_runeinfo;

typedef struct { /* 4 * 256 * 2 = 2048 (2KB) */
  l_runeinfo a[256];
} l_runetable;

typedef struct {
  l_runetable* t; /* table array, 1 table contains 1 rune, the array size is up to the length of the string */
  int size; /* a string map can store strings up to 'maxnumofstr', the size is the length of the longest string */
  int maxnumofstr;
} l_stringmap;

l_extern const l_stringmap* l_string_space_map();
l_extern const l_stringmap* l_string_newline_map();
l_extern const l_stringmap* l_string_blank_map();

l_extern l_string_map l_string_new_map(int maxstrlen, const l_rune** str, int numofstr, int casesensitive);
l_extern void l_string_set_map(l_stringmap* self, const l_rune** str, int numofstr, int casesensitive);
l_extern void l_string_free_map(l_stringmap* self);

l_extern const l_rune* l_string_match(const l_stringmap* map, const void* s, int len);
l_extern const l_rune* l_string_match_ex(const l_stringmap* map, const void* s, int len, int* strid, int* mlen);
l_extern const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, const void* s, int len);
l_extern const l_rune* l_string_match_repeat(const l_stringmap* map, const void* s, int len, int* lastmatchfailed);
l_extern const l_rune* l_string_match_until(const l_stringmap* map, const void* s, int len, int* n);
l_extern const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, const void* s, int len, int* n);
l_extern const l_rune* l_string_skip_space_and_match(const l_stringmap* map, const void* s, int len, int* strid, int* mlen);

l_extern void l_string_test();

#endif /* CCLIB_STRING_H_ */

