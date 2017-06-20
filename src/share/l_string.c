#include <string.h>
#include <stdarg.h>
#include "string.h"

l_strt l_strt_l(const void* s, l_integer len) {
  l_strt a = {0, 0};
  if (s && len > 0) {
    a.start = l_str(s);
    a.end = a.start + len;
  }
  return a;
}

int l_strt_contain(l_strt s, int ch) {
  while (s.start < s.end) {
    if (*s.start++ == ch) return true;
  }
  return false;
}

int l_strt_equal_l(l_strt s, const void* p, l_integer len) {
  const l_byte* str = l_str(p);
  if (s.end - s.start != len) return false;
  while (s.start < s.end) {
    if (*s.start++ != *str++) return false;
  }
  return true;
}

int l_buffer_ensure_capacity(l_buffer** self, l_int size) {
  l_buffer* temp = 0;
  l_int limit = (*self)->limit;
  if (limit && size > limit) return false;
  temp = (l_buffer*)l_thread_ensure_bfsize((*self)->belong, &(*self)->node, sizeof(l_buffer) + size);
  if (!temp) return false;
  *self = temp;
  return true;
}

int l_buffer_ensure_remain(l_buffer** self, l_int remainsize) {
  return l_buffer_ensure_capacity(self, (*self)->size + remainsize);
}

l_buffer* l_buffer_new(l_thread* thread, l_int initsize, l_int maxlimit) {
  l_buffer* p = 0;
  if (!thread) thread = l_thread_self();
  if (initsize < 32) initsize = 32;
  p = (l_buffer*)l_thread_alloc_buffer(thread, initsize);
  *(l_buffer_start(p)) = 0; /* zero terminated */
  p->belong = thread;
  p->size = 0;
  p->limit = (maxlimit <= 0) ? 0 : maxlimit;
  return p;
}

void l_buffer_free(l_buffer* p) {
  l_thread_free_buffer((*p)->belong, p, 0);
}

l_string l_string_new(l_strt from) {
  return l_thread_string_new(0, from);
}

l_string l_thread_string_new(l_thread* thread, l_strt from) {
  l_int size = from.end - from.start;
  l_buffer* p = l_buffer_new(thread, size+1, 0);
  l_copy_l(from.start, size, l_buffer_gets(p));
  p->size = size;
  *(l_buffer_gets(p) + size) = 0; /* zero terminated */
  return (l_string){p};
}

void l_string_free(l_string* self) {
  if (self->b) {
    l_buffer_free(self->b);
    self->b = 0;
  }
}

void l_string_clear(l_string* self) {
  l_buffer* p = self->b;
  p->size = 0;
  *l_buffer_gets(p) = 0;
}

void l_string_setstrt(l_string* self, l_strt s) {
  l_string_setlstr(self, s.start, s.end - s.start);
}

void l_string_setcstr(l_string* self, const void* s) {
  l_string_setlstr(self, s, strlen((const char*)s));
}

void l_string_setlstr(l_string* self, const void* s, l_int len) {
  l_buffer_ensure_capacity(&self->b, len+1);
  l_copy_l(s, len, l_buffer_gets(p));
  p->size = size;
  *(l_buffer_gets(p) + size) = 0;
}

int l_string_is_empty(l_string* self) {
  return (self->b->size == 0);
}

l_int l_string_size(l_string* self) {
  return (self->b->size);
}

const l_rune* l_string_cstr(l_string* self) {
  return l_buffer_gets(self->d);
}

int l_string_equal_c(l_string* self, const void* s) {
  return l_string_equal_l(self, s, strlen((const char*)s));
}

int l_string_equal_l(l_string* self, const void* s, l_int len) {
  const l_rune* a = l_string_cstr();
  const l_rune* b = l_str(s);
  if (l_string_size(self) != len) return false;
  while (len-- > 0) {
    if (a[len] != b[len]) return false;
  }
  return true;
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

static l_umedit l_right_most_bit(l_umedit n) {
  return n & (-n);
}

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

