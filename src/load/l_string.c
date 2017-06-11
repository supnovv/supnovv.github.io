#include "l_string.h"

#define L_BLANK_MAX_LEN (3)
#define L_NUM_OF_SPACES (23)
#define L_NUM_OF_NEWLINES (6)
#define L_NUM_OF_BLANKS (29)

static const l_rune* l_blanks[] = {
  L_STR("\x09"), /* \t */
  L_STR("\x0B"), /* \v */
  L_STR("\x0C"), /* \f */
  /* Zs 'Separator, Space' Category - www.fileformat.info/info/unicode/category/Zs/list.htm */
  L_STR("\x20"), /* 0x20 space */
  L_STR("\xC2\xA0"), /* 0xA0 no-break space */
  L_STR("\xE1\x9A\x80"), /* 0x1680 ogham space mark */
  L_STR("\xE2\x80\xAF"), /* 0x202F narrow no-break space */
  L_STR("\xE2\x81\x9F"), /* 0x205F medium mathematical space */
  L_STR("\xE3\x80\x80"), /* 0x3000 ideographic space (chinese blank character) */
  L_STR("\xE2\x80\x80"), /* 0x2000 en quad */
  L_STR("\xE2\x80\x81"), /* 0x2001 em quad */
  L_STR("\xE2\x80\x82"), /* 0x2002 en space */
  L_STR("\xE2\x80\x83"), /* 0x2003 em space */
  L_STR("\xE2\x80\x84"), /* 0x2004 three-per-em space */
  L_STR("\xE2\x80\x85"), /* 0x2005 four-per-em space */
  L_STR("\xE2\x80\x86"), /* 0x2006 six-per-em space */
  L_STR("\xE2\x80\x87"), /* 0x2007 figure space */
  L_STR("\xE2\x80\x88"), /* 0x2008 punctuation space */
  L_STR("\xE2\x80\x89"), /* 0x2009 thin space */
  L_STR("\xE2\x80\x8A"), /* 0x200A hair space */
  /* byte order marks */
  L_STR("\xFE\xFF"),
  L_STR("\xFF\xFE"),
  L_STR("\xEF\xBB\xBF"),
  /* new lines */
  L_STR("\x0A\x0D"), /* \n\r */
  L_STR("\x0D\x0A"), /* \r\n */
  L_STR("\x0A"), /* \r */
  L_STR("\x0D"), /* \n */
  L_STR("\xE2\x80\xA8"), /* line separator 0x2028 00100000_00101000 -> 1110'0010_10'000000_10'101000 (0xE280A8) */
  L_STR("\xE2\x80\xA9"), /* paragraph separator 0x2029 00100000_00101001 */
  0
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

const ccstringmap* l_string_newline_map() {
  ccstringmap* map = &ccg_newline_map;
  if (map->t) return map;
  *map = l_string_new_map(L_BLANK_MAX_LEN, l_blanks + L_NUM_OF_SPACES, L_NUM_OF_NEWLINES, true);
  return map;
}

const l_stringmap* l_string_blank_map() {
  l_stringmap* map = &l_blank_map;
  if (map->t) return map;
  *map = l_string_new_map(L_BLANK_MAX_LEN, l__blanks, L_NUM_OF_BLANKS, true);
  return map;
}

/* rune classes
%a - all letters              %A - the complement of %a
%d - all digits               %D
%x - all hexadecimal digits   %X
%l - all lowercase letters    %L
%u - all uppercase letters    %U
%. - all runes
%% - '%'
%[ - '['
%] - ']'
%^ - '^'
[chars] - rune set
[^chars] - the complement of [chars]
*/

void l_string_set_map(ccstringmap* self, const ccnauty_char** str, int numofstr, int casesensitive) {
  int stridx = 0, charidx = 0;
  const ccnauty_char* s = 0;
  ccchartable* t = self->t;
  ccnauty_char ch = 0;

  cczerol(self->t, sizeof(ccchartable) * self->size);

  for (; stridx < numofstr; ++stridx) {

    if (stridx + 1 > self->maxnumofstr) {
      l_log_error("too many strings");
      break;
    }

    s = str[stridx];
    if (s == 0 || s[0] == 0) continue;

    charidx = 0;
    while ((ch = s[charidx])) {

      if (charidx + 1 > self->size) {
        l_log_error("string too long");
        ++charidx;
        break;
      }

      /* the string match this char */
      t[charidx].c[ch].m |= (1 << stridx);
      if (!casesensitive) {
        if (ch >= 'a' && ch <= 'z') t[charidx].c[ch-32].m |= (1 << stridx);
        else if (ch >= 'A' && ch <= 'Z') t[charidx].c[ch+32].m |= (1 << stridx);
      }

      ++charidx;
    }

    /* the string ended with this char */
    ch = s[--charidx];
    t[charidx].c[ch].e |= (1 << stridx);
    if (!casesensitive) {
      if (ch >= 'a' && ch <= 'z') t[charidx].c[ch-32].e |= (1 << stridx);
      else if (ch >= 'A' && ch <= 'Z') t[charidx].c[ch+32].e |= (1 << stridx);
    }
  }
}

ccstringmap ccstring_newmap(int maxstrlen, const ccnauty_char** str, int numofstr, int casesensitive) {
  ccstringmap map = {0};
  if (maxstrlen <= 0 || str == 0) return map;
  map.t = (ccchartable*)ccrawalloc(sizeof(ccchartable) * maxstrlen);
  map.size = maxstrlen;
  map.maxnumofstr = 32;
  ccstring_setmap(&map, str, numofstr, casesensitive);
  return map;
}

void ccstring_freemap(ccstringmap* self) {
  if (self->t == 0) return;
  ccrawfree(self->t);
  self->t = 0;
  self->size = 0;
}

static const ccnauty_char* const ccstringtooshort = (const ccnauty_char* const)(ccnauty_iptr)(-1);

static ccmedit_uint ccrightmostbit(ccmedit_uint n) {
  return n & (-n);
}

static int ccpoweroftwobitpos(ccmedit_uint n) {
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
  ccassert(!"invalid number");
  return 0;
}

/* return 0 doesn't match, -1 too short, >s match success */
const ccnauty_char* ccstring_matchex(const ccstringmap* map, const void* s, int len, int* strid, int* mlen) {
  umedit_int prevmatch = 0xFFFFFFFF, curmatch = 0;
  umedit_int matches = 0, headmatch = 0;
  ccchartable* t = 0;
  int i = 0, end = len;
  nauty_byte ch = 0;
  const ccnauty_char* p = 0;

  /* example - (1) aabbcc (2) aabb (3) aa (4)zzbbcc
  (a) curmatch = 0b0111;
  (a) curmatch = 0b0111; t->e['a'] = 0b0100; headmatch = 0b0001;
  (b) curmatch = 0b0011;
  (b) curmatch = 0b0011; t->e['b'] = 0b1010; headmatch = 0b0001;
  (c) curmatch = 0b0001;
  (c) curmatch = 0b0001; t->e['c'] = 0b1001; headmatch = 0b0001;
  */

  if (len <= 0) {
    return ccstringtooshort;
  }

  if (map->size < len) {
    end = map->size;
  }

  while (i < end) {
    t = map->t + i;
    ch = CCSTR(s)[i];

    curmatch = prevmatch & t->c[ch].m;
    if (!curmatch) {
      if (p) {
        headmatch = ccrightmostbit(matches);
        goto MatchSuccess;
      }
      return 0;
    }

    if (t->c[ch].e & curmatch) {
      p = CCSTR(s) + i + 1;
      matches = t->c[ch].e & curmatch;
      headmatch = ccrightmostbit(curmatch);
      if (matches & headmatch) {
        goto MatchSuccess;
      }
    }

    prevmatch = curmatch;
    ++i;
  }

  if (p) {
    headmatch = ccrightmostbit(matches);
    goto MatchSuccess;
  }

  return ccstringtooshort; /* too short to match */

MatchSuccess:
  if (strid) *strid = ccpoweroftwobitpos(headmatch);
  if (mlen) *mlen = p - CCSTR(s);
  return p;
}

const ccnauty_char* ccstring_match(const ccstringmap* map, const void* s, int len) {
  return ccstring_matchex(map, s, len, 0, 0);
}

/* match exactly n times, return >=s or ccstringtooshort */
const ccnauty_char* ccstring_matchtimes(const ccstringmap* map, int n, const void* s, int len) {
  const ccnauty_char* e = CCSTR(s);
  int i = 0;

  while (i++ < n) {
    e = ccstring_match(map, s, len);
    if (e == 0) {
      return CCSTR(s);
    }
    if (e == ccstringtooshort) {
      return e;
    }
    s = e;
    len -= e - CCSTR(s);
  }

  return e;
}

/* return >=s */
const ccnauty_char* ccstring_matchrepeat(const ccstringmap* map, const void* s, int len, int* lastmatchfailed) {
  const ccnauty_char* e = 0;
  const ccnauty_char* prev = CCSTR(s);

  while ((e = ccstring_match(map, s, len)) != 0 && e != ccstringtooshort) {
    prev = e;
    s = e;
    len -= e - CCSTR(s);
  }

  if (e == 0) {
    if (lastmatchfailed) *lastmatchfailed = 1;
  } else {
    if (lastmatchfailed) *lastmatchfailed = 0;
  }

  return prev;
}

/* return 0 - too short to match, >=s - match success, n is the length */
const ccnauty_char* ccstring_matchuntil(const ccstringmap* map, const void* s, int len, int* n) {
  const ccnauty_char* e = 0;
  const ccnauty_char* cur = CCSTR(s);

  while ((e = ccstring_match(map, cur, len)) == 0) { /* continue only doesn't match */
    ++cur;
    --len;
  }

  if (e == ccstringtooshort) {
    if (n) *n = cur - CCSTR(s);
    return 0;
  }

  if (n) *n = e - cur;
  return e;
}

const ccnauty_char* ccstring_skipspaceandmatchuntil(const ccstringmap* map, const void* s, int len, int* n) {
  const ccnauty_char* e = 0;

  /* match space and skip */
  while ((e = ccstring_match(ccgetspacemap(), s, len)) != 0 && e != ccstringtooshort) {
    s = CCSTR(s) + 1;
    --len;
  }

  if (e == ccstringtooshort) {
    return 0; /* all chars are spaces, or even last space is not complete */
  }

  /* current char is not a space, try match the string */
  return ccstring_matchuntil(map, s, len, n);
}

/* return 0 - match failed; otherwise success, strid and mlen are set */
const ccnauty_char* ccstring_skipspaceandmatch(const ccstringmap* map, const void* s, int len, int* strid, int* mlen) {
  const ccnauty_char* e = 0;

  /* match space and skip */
  while ((e = ccstring_match(ccgetspacemap(), s, len)) != 0 && e != ccstringtooshort) {
    s = CCSTR(s) + 1;
    --len;
  }

  if (e == ccstringtooshort) {
    return 0; /* all chars are spaces, or even last space is not complete */
  }

  /* current char is not a space, try match the string */
  e = ccstring_matchex(map, s, len, strid, mlen);
  if (e == 0 || e == ccstringtooshort) {
    return 0;
  }
  return e;
}

void ccstringtest() {
  const ccnauty_char* methods[] = {CCSTR("GET"), CCSTR("HEAD"), CCSTR("POST")};
  const ccnauty_char* orderedchice[] = {CCSTR("mankind"), CCSTR("man"), CCSTR("got"),
      CCSTR("gotten"), CCSTR("pick"), CCSTR("tick"), CCSTR("cook")};
  const nauty_char subject[] = "gEtoHEAdopOStogeoend";
  const nauty_char* s = 0;
  ccstringmap map;

  ccassert(ccrightmostbit(0x0000) == 0x0000);
  ccassert(ccrightmostbit(0x0001) == 0x0001);
  ccassert(ccrightmostbit(0x0010) == 0x0010);
  ccassert(ccrightmostbit(0x0011) == 0x0001);
  ccassert(ccrightmostbit(0x0100) == 0x0100);
  ccassert(ccrightmostbit(0x0101) == 0x0001);
  ccassert(ccrightmostbit(0x0110) == 0x0010);
  ccassert(ccrightmostbit(0x0111) == 0x0001);
  ccassert(ccrightmostbit(0x1000) == 0x1000);
  ccassert(ccrightmostbit(0x1001) == 0x0001);
  ccassert(ccrightmostbit(0x1010) == 0x0010);
  ccassert(ccrightmostbit(0x1011) == 0x0001);
  ccassert(ccrightmostbit(0x1100) == 0x0100);
  ccassert(ccrightmostbit(0x1101) == 0x0001);
  ccassert(ccrightmostbit(0x1110) == 0x0010);
  ccassert(ccrightmostbit(0x1111) == 0x0001);

  map = ccstring_newmap(8, methods, 3, false);
  s = ccstring_match(&map, subject, 3);
  cclogd("get - %s", s);
  s = ccstring_match(&map, subject+1, 3);
  ccassert(s == 0);
  s = ccstring_match(&map, subject+4, 3);
  ccassert(s == ccstringtooshort);
  s = ccstring_match(&map, subject+4, 4);
  cclogd("head - %s", s);
  s = ccstring_match(&map, subject+9, 3);
  ccassert(s == ccstringtooshort);
  s = ccstring_match(&map, subject+9, 5);
  cclogd("post - %s", s);
  s = ccstring_match(&map, subject+14, 3);
  ccassert(s == 0);

  ccstring_setmap(&map, orderedchice, 7, true);
  s = ccstring_match(&map, "mankind", 7);
  ccassert(*s == 0);
  s = ccstring_match(&map, "mankin", 6);
  ccassert(*s == 'k');
  s = ccstring_match(&map, "gotten", 6);
  ccassert(*s == 't');
  s = ccstring_match(&map, "pon", 3);
  ccassert(s == 0);
  s = ccstring_match(&map, "con", 3);
  ccassert(s == 0);
  s = ccstring_match(&map, "tiok", 4);
  ccassert(s == 0);
  s = ccstring_match(&map, "tick", 4);
  ccassert(*s == 0);

  ccstring_freemap(&map);
}

