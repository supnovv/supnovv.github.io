#include <stdio.h>
#include "thatcore.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"
#include "service.h"

#define MATCH_OPOR_MAX_CHARS (64)

struct ccchartable {
  umedit_int t[256];
  umedit_int e[256];
};

umedit_int rightmostbitofone(umedit_int n) {
  return n & (-n);
}

#define HTTP_METHOD_MAX_CHARS (8)
static struct ccchartable methodmapset[HTTP_METHOD_MAX_CHARS];

const char* ccspaces[] = {
  "\x09", /* \t */
  "\x0B", /* \v */
  "\x0C", /* \f */
  /* Zs 'Separator, Space' Category - www.fileformat.info/info/unicode/category/Zs/list.htm */
  "\x20", /* 0x20 space */
  "\xC2\xA0", /* 0xA0 no-break space */
  "\xE1\x9A\x80", /* 0x1680 ogham space mark */
  "\xE2\x80\xAF", /* 0x202F narrow no-break space */
  "\xE2\x81\x9F", /* 0x205F medium mathematical space */
  "\xE3\x80\x80", /* 0x3000 ideographic space (chinese blank character) */
  "\xE2\x80\x80", /* 0x2000 en quad */
  "\xE2\x80\x81", /* 0x2001 em quad */
  "\xE2\x80\x82", /* 0x2002 en space */
  "\xE2\x80\x83", /* 0x2003 em space */
  "\xE2\x80\x84", /* 0x2004 three-per-em space */
  "\xE2\x80\x85", /* 0x2005 four-per-em space */
  "\xE2\x80\x86", /* 0x2006 six-per-em space */
  "\xE2\x80\x87", /* 0x2007 figure space */
  "\xE2\x80\x88", /* 0x2008 punctuation space */
  "\xE2\x80\x89", /* 0x2009 thin space */
  "\xE2\x80\x8A", /* 0x200A hair space */
  /* byte order marks */
  "\xFE\xFF",
  "\xFF\xFE",
  "\xEF\xBB\xBF",
  0
};

const char* ccnewlines[] = {
  "\x0A\x0D", /* \n\r */
  "\x0D\x0A", /* \r\n */
  "\x0A", /* \r */
  "\x0D", /* \n */
  "\xE2\x80\xA8", /* line separator 0x2028 00100000_00101000 -> 1110'0010_10'000000_10'101000 (0xE280A8) */
  "\xE2\x80\xA9", /* paragraph separator 0x2029 00100000_00101001 */
  0
};

const char* ccblanks[] = {
  "\x09", /* \t */
  "\x0B", /* \v */
  "\x0C", /* \f */
  /* Zs 'Separator, Space' Category - www.fileformat.info/info/unicode/category/Zs/list.htm */
  "\x20", /* 0x20 space */
  "\xC2\xA0", /* 0xA0 no-break space */
  "\xE1\x9A\x80", /* 0x1680 ogham space mark */
  "\xE2\x80\xAF", /* 0x202F narrow no-break space */
  "\xE2\x81\x9F", /* 0x205F medium mathematical space */
  "\xE3\x80\x80", /* 0x3000 ideographic space (chinese blank character) */
  "\xE2\x80\x80", /* 0x2000 en quad */
  "\xE2\x80\x81", /* 0x2001 em quad */
  "\xE2\x80\x82", /* 0x2002 en space */
  "\xE2\x80\x83", /* 0x2003 em space */
  "\xE2\x80\x84", /* 0x2004 three-per-em space */
  "\xE2\x80\x85", /* 0x2005 four-per-em space */
  "\xE2\x80\x86", /* 0x2006 six-per-em space */
  "\xE2\x80\x87", /* 0x2007 figure space */
  "\xE2\x80\x88", /* 0x2008 punctuation space */
  "\xE2\x80\x89", /* 0x2009 thin space */
  "\xE2\x80\x8A", /* 0x200A hair space */
  /* byte order marks */
  "\xFE\xFF",
  "\xFF\xFE",
  "\xEF\xBB\xBF",
  /* new lines */
  "\x0A\x0D", /* \n\r */
  "\x0D\x0A", /* \r\n */
  "\x0A", /* \r */
  "\x0D", /* \n */
  "\xE2\x80\xA8", /* line separator 0x2028 00100000_00101000 -> 1110'0010_10'000000_10'101000 (0xE280A8) */
  "\xE2\x80\xA9", /* paragraph separator 0x2029 00100000_00101001 */
  0
};

struct ccstringmap {
  struct ccchartable* set;
  int size;
};

static struct ccchartable ll_space_map[4];
static struct ccstringmap ccspacemap = {ll_space_map, 4};

void ccstring_setmap(struct ccchartable* set, int size, const char** orlist, int casesensitive) {
  int stridx = 0, charidx = 0;
  const char* s = 0;
  nauty_byte ch = 0;

  cczeron(set, sizeof(struct ccchartable)*size);

  for (; (s = orlist[stridx]); ++stridx) {

    if (stridx+1 > 32) {
      ccloge("too many strings in orlist");
      break;
    }

    /* "or" a empty string take no effect */
    if (s[0] == 0) {
      continue;
    }

    charidx = 0;
    while ((ch = s[charidx])) {

      if (charidx+1 > size) {
        ccloge("too many chars in string");
        ++charidx;
        break;
      }

      /* each char in the string has a table */
      set[charidx].t[ch] |= (1 << stridx);
      if (!casesensitive) {
        if (ch >= 'a' && ch <= 'z') set[charidx].t[ch-32] |= (1 << stridx);
        else if (ch >= 'A' && ch <= 'Z') set[charidx].t[ch+32] |= (1 << stridx);
      }

      ++charidx;
    }

    /* the string ended with this char */
    ch = s[--charidx];
    set[charidx].e[ch] |= (1 << stridx);
    if (!casesensitive) {
      if (ch >= 'a' && ch <= 'z') set[charidx].e[ch-32] |= (1 << stridx);
      else if (ch >= 'A' && ch <= 'Z') set[charidx].e[ch+32] |= (1 << stridx);
    }
  }
}

static const char* const ccstringtooshort = (const char* const)(signed_ptr)(-1);

static int ccpowtoindex(umedit_int n) {
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
const char* ccstring_matchex(struct ccstringmap* ormap, const char* s, int len, int* strid, int* mlen) {
  umedit_int prevmatch = 0xFFFFFFFF, curmatch = 0;
  umedit_int matches = 0, headmatch = 0;
  struct ccchartable* set = 0;
  int i = 0, end = len;
  nauty_byte ch = 0;
  const char* p = 0;

  /* example - (1) aabbcc (2) aabb (3) aa (4)zzbbcc
  (a) curmatch = 0b0111;
  (a) curmatch = 0b0111; set->e['a'] = 0b0100; headmatch = 0b0001;
  (b) curmatch = 0b0011;
  (b) curmatch = 0b0011; set->e['b'] = 0b1010; headmatch = 0b0001;
  (c) curmatch = 0b0001;
  (c) curmatch = 0b0001; set->e['c'] = 0b1001; headmatch = 0b0001;
  */

  if (len <= 0) {
    return ccstringtooshort;
  }

  if (ormap->size < len) {
    end = ormap->size;
  }

  while (i < end) {
    set = ormap->set + i;
    ch = s[i];

    curmatch = prevmatch & set->t[ch];
    if (!curmatch) {
      if (p) {
        headmatch = matches & (-matches);
        goto MatchSuccess;
      }
      return 0;
    }

    if (set->e[ch] & curmatch) {
      p = s + i + 1;
      matches = set->e[ch] & curmatch;
      headmatch = curmatch & (-curmatch); /* ordered choice */
      if (matches & headmatch) {
        goto MatchSuccess;
      }
    }

    prevmatch = curmatch;
    ++i;
  }

  if (p) {
    headmatch = matches & (-matches);
    goto MatchSuccess;
  }

  return ccstringtooshort; /* too short to match */

MatchSuccess:
  if (strid) *strid = ccpowtoindex(headmatch);
  if (mlen) *mlen = p - s;
  return p;
}

const char* ccstring_match(struct ccstringmap* ormap, const char* s, int len) {
  return ccstring_matchex(ormap, s, len, 0, 0);
}

/* match exactly n times, return >=s or ccstringtooshort */
const char* ccstring_matchtimes(struct ccstringmap* ormap, int n, const char* s, int len) {
  const char* e = s;
  int i = 0;

  while (i++ < n) {
    e = ccstring_match(ormap, s, len);
    if (e == 0) {
      return s;
    }
    if (e == ccstringtooshort) {
      return e;
    }
    s = e;
    len -= e - s;
  }

  return e;
}

/* return >=s */
const char* ccstring_matchrepeat(struct ccstringmap* ormap, const char* s, int len, int* lasttimematchfailed) {
  const char* e = 0;
  const char* prev = s;

  while ((e = ccstring_match(ormap, s, len)) != 0 && e != ccstringtooshort) {
    prev = e;
    s = e;
    len -= e - s;
  }

  if (e == 0) {
    if (lasttimematchfailed) *lasttimematchfailed = 1;
  } else {
    if (lasttimematchfailed) *lasttimematchfailed = 0;
  }

  return prev;
}

/* return 0 - too short to match, >=s - match success, n is the length */
const char* ccstring_matchuntil(struct ccstringmap* ormap, const char* s, int len, int* n) {
  const char* e = 0;
  const char* cur = s;

  while ((e = ccstring_match(ormap, cur, len)) == 0) { /* continue only doesn't match */
    ++cur;
    --len;
  }

  if (e == ccstringtooshort) {
    if (n) *n = cur - s;
    return 0;
  }

  if (n) *n = e - cur;
  return e;
}

/* return 0 - match failed; otherwise success, strid and mlen are set */
const char* ccstring_skipspaceandmatch(struct ccstringmap* ormap, const char* s, int len, int* strid, int* mlen) {
  const char* e = 0;

  /* match space and skip */
  while ((e = ccstring_match(&ccspacemap, s, len)) != 0 && e != ccstringtooshort) {
    ++s;
    --len;
  }

  if (e == ccstringtooshort) {
    return 0; /* all chars are spaces, or even last space is not complete */ 
  }

  /* current char is not a space, try match the string */
  e = ccstring_matchex(ormap, s, len, strid, mlen);
  if (e == 0 || e == ccstringtooshort) {
    return 0;
  }
  return e;
}

static const char* cchttpmethods[] = {
 "GET", "HEAD", "POST", 0
};

static const char* orderedchice[] = {
  "mankind", "man", "got", "gotten", "pick", "tick", "cook", 0 
};

void matchtest() {
  const char subject[] = "gEtoHEAdopOStogeoend";
  const char* s = 0;
  struct ccstringmap ormap;

  ccstring_setmap(methodmapset, HTTP_METHOD_MAX_CHARS, cchttpmethods, false);
  ormap.set = methodmapset;
  ormap.size = HTTP_METHOD_MAX_CHARS;
  s = ccstring_match(&ormap, subject, 3);
  printf("get - %s\n", s);
  s = ccstring_match(&ormap, subject+1, 3);
  ccassert(s == 0);
  s = ccstring_match(&ormap, subject+4, 3);
  ccassert(s == ccstringtooshort);
  s = ccstring_match(&ormap, subject+4, 4);
  printf("head - %s\n", s);
  s = ccstring_match(&ormap, subject+9, 3);
  ccassert(s == ccstringtooshort);
  s = ccstring_match(&ormap, subject+9, 5);
  printf("post - %s\n", s);
  s = ccstring_match(&ormap, subject+14, 3);
  ccassert(s == 0);

  ccstring_setmap(methodmapset, HTTP_METHOD_MAX_CHARS, orderedchice, true);
  s = ccstring_match(&ormap, "mankind", 7);
  ccassert(*s == 0);
  s = ccstring_match(&ormap, "mankin", 6);
  ccassert(*s == 'k');
  s = ccstring_match(&ormap, "gotten", 6);
  ccassert(*s == 't');
  s = ccstring_match(&ormap, "pon", 3);
  ccassert(s == 0);
  s = ccstring_match(&ormap, "con", 3);
  ccassert(s == 0);
  s = ccstring_match(&ormap, "tiok", 4);
  ccassert(s == 0);
  s = ccstring_match(&ormap, "tick", 4);
  ccassert(*s == 0);
}

void bitoptest() {
  ccassert(rightmostbitofone(0x0000) == 0x0000);
  ccassert(rightmostbitofone(0x0001) == 0x0001);
  ccassert(rightmostbitofone(0x0010) == 0x0010);
  ccassert(rightmostbitofone(0x0011) == 0x0001);
  ccassert(rightmostbitofone(0x0100) == 0x0100);
  ccassert(rightmostbitofone(0x0101) == 0x0001);
  ccassert(rightmostbitofone(0x0110) == 0x0010);
  ccassert(rightmostbitofone(0x0111) == 0x0001);
  ccassert(rightmostbitofone(0x1000) == 0x1000);
  ccassert(rightmostbitofone(0x1001) == 0x0001);
  ccassert(rightmostbitofone(0x1010) == 0x0010);
  ccassert(rightmostbitofone(0x1011) == 0x0001);
  ccassert(rightmostbitofone(0x1100) == 0x0100);
  ccassert(rightmostbitofone(0x1101) == 0x0001);
  ccassert(rightmostbitofone(0x1110) == 0x0010);
  ccassert(rightmostbitofone(0x1111) == 0x0001);
}

int cctest_start() {
  ccsetloglevel(4);
  ccthattest();
  ccluatest();
  ccplattest();
  ccplationftest();
  ccplatsocktest();
  matchtest();
  bitoptest();
  return 0;
}

int main() {
  return startmainthread(cctest_start);
}

