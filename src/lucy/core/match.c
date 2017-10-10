#include <stdio.h>

#define L_LIBRARY_IMPL
#include "core/string.h"
#include "core/match.h"

#define L_BLANK_MAX_LEN (3)
#define L_NUM_OF_SPACES (23)
#define L_NUM_OF_NEWLINES (6)
#define L_NUM_OF_BLANKS (29)

typedef struct {
  l_umedit m; /* string match this char */
  l_umedit e; /* string ended with this char */
} l_runeinfo;

typedef struct { /* 4 * 256 * 2 = 2048 (2KB) */
  l_runeinfo a[256];
} l_runetable;

L_GLOBAL const l_strn l_blanks[] = {
  l_strn_literal("\x09"), /* \t */
  l_strn_literal("\x0b"), /* \v */
  l_strn_literal("\x0c"), /* \f */
  /* Zs 'Separator, Space' Category - www.fileformat.info/info/unicode/category/Zs/list.htm */
  l_strn_literal("\x20"), /* 0x20 space */
  l_strn_literal("\xc2\xa0"), /* 0xA0 no-break space */
  l_strn_literal("\xe1\x9a\x80"), /* 0x1680 ogham space mark */
  l_strn_literal("\xe2\x80\xaf"), /* 0x202F narrow no-break space */
  l_strn_literal("\xe2\x81\x9f"), /* 0x205F medium mathematical space */
  l_strn_literal("\xe3\x80\x80"), /* 0x3000 ideographic space (chinese blank character) */
  l_strn_literal("\xe2\x80\x80"), /* 0x2000 en quad */
  l_strn_literal("\xe2\x80\x81"), /* 0x2001 em quad */
  l_strn_literal("\xe2\x80\x82"), /* 0x2002 en space */
  l_strn_literal("\xe2\x80\x83"), /* 0x2003 em space */
  l_strn_literal("\xe2\x80\x84"), /* 0x2004 three-per-em space */
  l_strn_literal("\xe2\x80\x85"), /* 0x2005 four-per-em space */
  l_strn_literal("\xe2\x80\x86"), /* 0x2006 six-per-em space */
  l_strn_literal("\xe2\x80\x87"), /* 0x2007 figure space */
  l_strn_literal("\xe2\x80\x88"), /* 0x2008 punctuation space */
  l_strn_literal("\xe2\x80\x89"), /* 0x2009 thin space */
  l_strn_literal("\xe2\x80\x8a"), /* 0x200A hair space */
  /* byte order marks */
  l_strn_literal("\xfe\xff"),
  l_strn_literal("\xff\xfe"),
  l_strn_literal("\xef\xbb\xbf"),
  /* new lines */
  l_strn_literal("\x0a\x0d"), /* \n\r */
  l_strn_literal("\x0d\x0a"), /* \r\n */
  l_strn_literal("\x0a"), /* \r */
  l_strn_literal("\x0d"), /* \n */
  l_strn_literal("\xe2\x80\xa8"), /* line separator 0x2028 00100000_00101000 -> 1110'0010_10'000000_10'101000 (0xE280A8) */
  l_strn_literal("\xe2\x80\xa9")  /* paragraph separator 0x2029 00100000_00101001 */
};

L_GLOBAL l_stringmap l_space_map; /* used to match a space */
L_GLOBAL l_stringmap l_newline_map; /* used to match a newline */
L_GLOBAL l_stringmap l_blank_map; /* used to match a blank, blank is a space or a newline */

L_EXTERN const l_stringmap*
l_stringmap_space()
{
  l_stringmap* map = &l_space_map;
  if (map->t) return map;
  *map = l_stringmap_new(L_BLANK_MAX_LEN, l_blanks, L_NUM_OF_SPACES, true);
  return map;
}

L_EXTERN const l_stringmap*
l_stringmap_newline()
{
  l_stringmap* map = &l_newline_map;
  if (map->t) return map;
  *map = l_stringmap_new(L_BLANK_MAX_LEN, l_blanks + L_NUM_OF_SPACES, L_NUM_OF_NEWLINES, true);
  return map;
}

L_EXTERN const l_stringmap*
l_stringmap_blank()
{
  l_stringmap* map = &l_blank_map;
  if (map->t) return map;
  *map = l_stringmap_new(L_BLANK_MAX_LEN, l_blanks, L_NUM_OF_BLANKS, true);
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

L_EXTERN void
l_stringmap_set(l_stringmap* self, const l_strn* str, l_int numofstr, int casesensitive)
{
  int stridx = 0, charidx = 0;
  const l_byte* s = 0;
  l_runetable* t = (l_runetable*)(self->t);
  l_byte ch = 0;

  l_zero_n(self->t, sizeof(l_runetable) * self->size);

  for (; stridx < numofstr; ++stridx) {

    if (stridx + 1 > self->strlimit) {
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

L_EXTERN l_stringmap
l_stringmap_new(l_int maxstrlen, const l_strn* str, l_int numofstr, int casesensitive)
{
  l_stringmap map = {0};
  if (maxstrlen <= 0 || str == 0) return map;
  map.t = (l_runetable*)l_raw_malloc(sizeof(l_runetable) * maxstrlen);
  map.size = maxstrlen;
  map.strlimit = 32;
  l_stringmap_set(&map, str, numofstr, casesensitive);
  return map;
}

L_EXTERN void
l_stringmap_free(l_stringmap* self)
{
  if (self->t == 0) return;
  l_raw_mfree(self->t);
  self->t = 0;
  self->size = 0;
}

L_GLOBAL const l_byte* const l_string_too_short = (const l_byte* const)(l_int)(1);

static int
l_power_of_two_bit_pos(l_umedit n)
{
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
L_EXTERN const l_byte*
l_string_matchEx(const l_stringmap* map, l_strt s, l_int* strid, l_int* matched_len)
{
  l_umedit prevmatch = 0xFFFFFFFF, curmatch = 0;
  l_umedit matches = 0, headmatch = 0;
  l_runetable* t = 0;
  l_int i = 0, len = s.end - s.start;
  l_byte ch = 0;
  const l_byte* p = 0;

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

L_EXTERN const l_byte*
l_string_match(const l_stringmap* map, l_strt s)
{
  return l_string_matchEx(map, s, 0, 0);
}

/* match exactly n times, return 0 or l_string_too_short for fail */
L_EXTERN const l_byte*
l_string_matchTimes(const l_stringmap* map, l_int n, l_strt s)
{
  const l_byte* e = s.start;
  l_int i = 0;
  while (i++ < n) {
    if ((e = l_string_match(map, s)) == 0 || e == l_string_too_short) return 0;
    s.start = e;
  }
  return e;
}

L_EXTERN const l_byte*
l_string_matchRepeat(const l_stringmap* map, l_strt s)
{
  const l_byte* e = 0;
  while ((e = l_string_match(map, s)) != 0 && e != l_string_too_short) {
    s.start = e;
  }
  return s.start;
}

/* return 0 - too short to match, otherwise success */
L_EXTERN const l_byte*
l_string_matchUntil(const l_stringmap* map, l_strt s, l_byte** last_match_start)
{
  const l_byte* e = 0;
  while ((e = l_string_match(map, s)) == 0) { /* continue loop when unmatched */
    ++s.start;
  }
  if (last_match_start) *last_match_start = (l_byte*)s.start;
  return (e == l_string_too_short ? 0 : e);
}

L_EXTERN const l_byte*
l_string_skipSpaceAndMatchUntil(const l_stringmap* map, l_strt s, l_byte** first_non_space_pos)
{
  const l_byte* e = 0;
  l_byte* last_match_start = 0;

  while ((e = l_string_match(l_stringmap_space(), s)) != 0 && e != l_string_too_short) {
    s.start = e; /* skip space */
  }

  if (e == l_string_too_short) return 0; /* last space is not complete */
  if (first_non_space_pos) *first_non_space_pos = (l_byte*)s.start;
  if (!(e = l_string_matchUntil(map, s, &last_match_start))) {
    return last_match_start;
  }
  return e;
}

/* return 0 - match failed; otherwise success, strid and mlen are set */
L_EXTERN const l_byte*
l_string_skipSpaceAndMatch(const l_stringmap* map, l_strt s, l_int* strid, l_int* matched_len)
{
  const l_byte* e = 0;
  while ((e = l_string_match(l_stringmap_space(), s)) != 0 && e != l_string_too_short) {
    s.start = e; /* skip space */
  }

  e = l_string_matchEx(map, s, strid, matched_len);
  if (e == 0 || e == l_string_too_short) return 0;
  return e;
}

L_EXTERN const l_byte*
l_string_trimHead(l_strt s)
{
  const l_byte* e = 0;
  while ((e = l_string_match(l_stringmap_space(), s)) != 0 && e != l_string_too_short) {
    s.start = e; /* skip space */
  }
  return s.start;
}

L_EXTERN const l_byte*
l_string_skipSpaceAndMatchSub(l_strt sub, l_strt s)
{
  const l_byte* e = 0;
  while ((e = l_string_match(l_stringmap_space(), s)) != 0 && e != l_string_too_short) {
    s.start = e; /* skip space */
  }

  if (s.end - s.start < sub.end - sub.start) return 0;
  while (sub.start < sub.end) {
    if (*sub.start++ != *s.start++) return 0;
  }
  return s.start;
}

L_EXTERN void
l_match_test()
{
  const l_strn methods[] = {l_strn_literal("GET"), l_strn_literal("HEAD"), l_strn_literal("POST")};
  const l_strn orderedchice[] = {l_strn_literal("mankind"), l_strn_literal("man"), l_strn_literal("got"),
      l_strn_literal("gotten"), l_strn_literal("pick"), l_strn_literal("tick"), l_strn_literal("cook")};
  const l_byte subject[] = "gEtoHEAdopOStogeoend";
  const l_byte* s = 0;
  l_stringmap map;
  l_string str = l_string_create(8);

  l_string_format_1(&str, "%f", lf(3.1415926));
  l_logd_1("%f", lf(3.1415926));
  printf("[D] %.80f\n", 3.1415926);
  l_assert(l_string_equal(&str, l_strt_literal("3.14159260000000006840537025709636509418487548828125")));

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

  map = l_stringmap_new(8, methods, 3, false);
  s = l_string_match(&map, l_strt_n(subject, 3));
  l_logd_1("get - %s", ls(s));
  s = l_string_match(&map, l_strt_n(subject+1, 3));
  l_assert(s == 0);
  s = l_string_match(&map, l_strt_n(subject+4, 3));
  l_assert(s == l_string_too_short);
  s = l_string_match(&map, l_strt_n(subject+4, 4));
  l_logd_1("head - %s", ls(s));
  s = l_string_match(&map, l_strt_n(subject+9, 3));
  l_assert(s == l_string_too_short);
  s = l_string_match(&map, l_strt_n(subject+9, 5));
  l_logd_1("post - %s", ls(s));
  s = l_string_match(&map, l_strt_n(subject+14, 3));
  l_assert(s == 0);

  l_stringmap_set(&map, orderedchice, 7, true);
  s = l_string_match(&map, l_strt_literal("mankind"));
  l_assert(*s == 0);
  s = l_string_match(&map, l_strt_literal("mankin"));
  l_assert(*s == 'k');
  s = l_string_match(&map, l_strt_literal("gotten"));
  l_assert(*s == 't');
  s = l_string_match(&map, l_strt_literal("pon"));
  l_assert(s == 0);
  s = l_string_match(&map, l_strt_literal("con"));
  l_assert(s == 0);
  s = l_string_match(&map, l_strt_literal("tiok"));
  l_assert(s == 0);
  s = l_string_match(&map, l_strt_literal("tick"));
  l_assert(*s == 0);

  l_stringmap_free(&map);
  l_string_free(&str, 0);
}

