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
  umedit_int curmatch = 0;

  struct cccharset* set = 0;
  int i = 0, end = len;
  nauty_byte ch = 0;

  if (ormap->size < len) {
    end = ormap->size;
  }

  while (i < end) {
    set = ormap->set + i;
    ch = s[i];

    curmatch = prevmatch & set->t[ch];
    if (!curmatch) {
      return 0;
    }

    if (set->e[ch]) {
      return s + i + 1;
    }

    prevmatch = curmatch;
    ++i;
  }

  /* too short to match */
  return ccstringtooshort;
}

static const char* cchttpmethods[] = {
 "GET", "HEAD", "POST", 0
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
}

int cctest_start() {
  ccsetloglevel(4);
  ccthattest();
  ccluatest();
  ccplattest();
  ccplationftest();
  ccplatsocktest();
  matchtest();
  return 0;
}

int main() {
  return startmainthread(cctest_start);
}

