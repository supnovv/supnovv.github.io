#include <stdio.h>
#include "thatcore.h"
#include "luacapi.h"
#include "ionotify.h"
#include "socket.h"
#include "service.h"

#define MATCH_OPOR_MAX_CHARS (64)

struct cccharset {
  umedit_int t[256];
  umedit_int e[256];
};

umedit_int rightmostbitofone(umedit_int n) {
  return n & (-n);
}

#define HTTP_METHOD_MAX_CHARS (8)
static struct cccharset methodmapset[HTTP_METHOD_MAX_CHARS];

struct stringormap {
  struct cccharset* set;
  int size;
};

void string_setormap(struct cccharset* set, int size, const char** orlist, int casesensitive) {
  int stridx = 0, charidx = 0;
  const char* s = 0;
  nauty_byte ch = 0;

  cczeron(set, sizeof(struct cccharset)*size);

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

const char* string_match(struct stringormap* ormap, const char* s, int len) {
  umedit_int prevmatch = 0xFFFFFFFF;
  umedit_int curmatch = 0, headmatch = 0;

  struct cccharset* set = 0;
  int i = 0, end = len;
  nauty_byte ch = 0;
  const char* p = 0;

  if (ormap->size < len) {
    end = ormap->size;
  }

  while (i < end) {
    set = ormap->set + i;
    ch = s[i];

    curmatch = prevmatch & set->t[ch];
    if (!curmatch) {
      return p;
    }

    if (set->e[ch] & curmatch) {
      p = s + i + 1;
      headmatch = curmatch & (-curmatch); /* ordered chice */
      if (set->e[ch] & headmatch) {
        return p;
      }
    }

    prevmatch = curmatch;
    ++i;
  }

  if (p) return p;
  return ccstringtooshort; /* too short to match */
}

/* match exactly n times, return >=s or ccstringtooshort */
const char* string_matchtimes(struct stringormap* ormap, int n, const char* s, int len) {
  const char* e = 0;
  int i = 0;
  if (n <= 0) return s;
  while (i++ < n) {
    e = string_match(ormap, s, len);
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
const char* string_matchrepeat(struct stringormap* ormap, const char* s, int len, int* lasttimematchfailed) {
  const char* e = 0;
  const char* prev = s;

  while ((e = string_match(ormap, s, len)) != 0 && e != ccstringtooshort) {
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

static const char* cchttpmethods[] = {
 "GET", "HEAD", "POST", 0
};

static const char* orderedchice[] = {
  "mankind", "man", "got", "gotten", "pick", "tick", "cook", 0 
};

void matchtest() {
  const char subject[] = "gEtoHEAdopOStogeoend";
  const char* s = 0;
  struct stringormap ormap;

  string_setormap(methodmapset, HTTP_METHOD_MAX_CHARS, cchttpmethods, false);
  ormap.set = methodmapset;
  ormap.size = HTTP_METHOD_MAX_CHARS;
  s = string_match(&ormap, subject, 3);
  printf("get - %s\n", s);
  s = string_match(&ormap, subject+1, 3);
  ccassert(s == 0);
  s = string_match(&ormap, subject+4, 3);
  ccassert(s == ccstringtooshort);
  s = string_match(&ormap, subject+4, 4);
  printf("head - %s\n", s);
  s = string_match(&ormap, subject+9, 3);
  ccassert(s == ccstringtooshort);
  s = string_match(&ormap, subject+9, 5);
  printf("post - %s\n", s);
  s = string_match(&ormap, subject+14, 3);
  ccassert(s == 0);

  string_setormap(methodmapset, HTTP_METHOD_MAX_CHARS, orderedchice, true);
  s = string_match(&ormap, "mankind", 7);
  ccassert(*s == 0);
  s = string_match(&ormap, "mankin", 6);
  ccassert(*s == 'k');
  s = string_match(&ormap, "gotten", 6);
  ccassert(*s == 't');
  s = string_match(&ormap, "pon", 3);
  ccassert(s == 0);
  s = string_match(&ormap, "con", 3);
  ccassert(s == 0);
  s = string_match(&ormap, "tiok", 4);
  ccassert(s == 0);
  s = string_match(&ormap, "tick", 4);
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

