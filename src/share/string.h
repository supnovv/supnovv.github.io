#ifndef CCLIB_STRING_H_
#define CCLIB_STRING_H_
#include "thatcore.h"

typedef struct { /* 4 * 256 * 2 = 2048 (2KB) */
  ccmedit_uint a[256]; /* string has this char */
  ccmedit_uint e[256]; /* string end with this char */
} ccchartable;

typedef struct {
  ccchartable* t; /* table array, 1 table contains 1 char, tha array size is up to the length of the string */
  int size; /* a string map can store strings up to 'maxnumofstr', the size is the length of the longest string */
  int maxnumofstr;
} ccstringmap;

CORE_API const ccstringmap* ccgetspacemap();
CORE_API const ccstringmap* ccgetnewlinemap();
CORE_API const ccstringmap* ccgetblankmap();

CORE_API ccstringmap ccstring_newmap(int maxstrlen, const ccnauty_char** str, int casesensitive);
CORE_API void ccstring_setmap(ccstringmap* self, const ccnauty_char** str, int casesensitive);
CORE_API void ccstring_freemap(ccstringmap* self);

CORE_API const ccnauty_char* ccstring_match(const ccstringmap* map, const void* s, int len);
CORE_API const ccnauty_char* ccstring_matchex(const ccstringmap* map, const void* s, int len, int* strid, int* mlen);
CORE_API const ccnauty_char* ccstring_matchtimes(const ccstringmap* map, int n, const void* s, int len);
CORE_API const ccnauty_char* ccstring_matchrepeat(const ccstringmap* map, const void* s, int len, int* lastmatchfailed);
CORE_API const ccnauty_char* ccstring_matchuntil(const ccstringmap* map, const void* s, int len, int* n);
CORE_API const ccnauty_char* ccstring_skipspaceandmatch(const ccstringmap* map, const void* s, int len, int* strid, int* mlen);

CORE_API void ccstringtest();

#endif /* CCLIB_STRING_H_ */

