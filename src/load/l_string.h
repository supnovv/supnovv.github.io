#ifndef l_string_lib_h
#define l_string_lib_h
#include "l_core.h"

#define CCSTRING_SIZEOF 32
#define CCSTRING_STATIC_CHARS 30

typedef struct ccstring {
  struct ccheap heap;
  sright_int len;
  uoctet_int a[CCSTRING_SIZEOF-sizeof(struct ccheap)-sizeof(sright_int)-1];
  uoctet_int flag; /* flag==0xFF ? heap-string : stack-string */
} ccstring; /* a sequence of bytes with zero terminated */

CORE_API const char* ccutos(uright_int a);
CORE_API const char* ccitos(sright_int a);
CORE_API ccstring ccemptystr();
CORE_API ccstring ccstrfromu(uright_int a);
CORE_API ccstring ccstrfromuf(uright_int a, sright_int fmt);
CORE_API ccstring ccstrfromi(sright_int a);
CORE_API ccstring ccstrfromif(sright_int a, sright_int fmt);

CORE_API ccstring ccstring_emptystr();
CORE_API sright_int ccstring_getlen(const ccstring* self);
CORE_API const char* ccstring_getcstr(const ccstring* self);
CORE_API nauty_bool ccstring_equalcstr(const ccstring* self, const void* cstr);

CORE_API void ccstring_free(ccstring* self);
CORE_API void ccstring_setempty(ccstring* self);
CORE_API void ccstring_setcstr(ccstring* self, const void* cstr);

CORE_API ccfrom ccstring_getfrom(const ccstring* s);
CORE_API nauty_bool ccstring_contain(struct ccfrom s, nauty_char ch);
CORE_API nauty_bool ccstring_containp(const struct ccfrom* s, nauty_char ch);

#if 0
CCINLINE nauty_char cclower(nauty_char ch) {
  return (ch >= 'A' && ch <= 'Z') ? (ch + 32) : ch;
}

CCINLINE void cclowerp(nauty_char* ch) {
  *ch = cclower(*ch);
}

CCINLINE nauty_char ccupper(nauty_char ch) {
  return (ch >= 'a' && ch <= 'z') ? (ch - 32) : ch;
}

CCINLINE void ccupperp(nauty_char* ch) {
  *ch = ccupper(*ch);
}

CCINLINE void ccstring_lower(nauty_char* s, int len) {
  while (len > 0) {
    cclowerp(s + (--len));
  }
}

CCINLINE void ccstring_upper(nauty_char* s, int len) {
  while (len > 0) {
    ccupperp(s + (--len));
  }
}

#endif

typedef struct {
  l_umedit m; /* string match this rune */
  l_umedit e; /* string ended at this rune */
} l_runeinfo;

typedef struct { /* 4 * 256 * 2 = 2048 (2KB) */
  l_runeinfo a[256];
} l_runetable;

typedef struct {
  l_runetable* t; /* table array, 1 table contains 1 rune, tha array size is up to the length of the string */
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
l_extern const l_rune* l_string_match_extern(const l_stringmap* map, const void* s, int len, int* strid, int* mlen);
l_extern const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, const void* s, int len);
l_extern const l_rune* l_string_match_repeat(const l_stringmap* map, const void* s, int len, int* lastmatchfailed);
l_extern const l_rune* l_string_match_until(const l_stringmap* map, const void* s, int len, int* n);
l_extern const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, const void* s, int len, int* n);
l_extern const l_rune* l_string_skip_space_and_match(const l_stringmap* map, const void* s, int len, int* strid, int* mlen);

l_extern void l_string_test();

#endif /* l_string_lib_h */

