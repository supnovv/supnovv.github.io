#ifndef l_core_match_h
#define l_core_match_h
#include "core/base.h"

typedef struct {
  void* t; /* table array, 1 table contains 1 char, the array size is up to the length of the string */
  l_int size; /* a string map can store strings up to 'strlimit', the size is the length of the longest string */
  l_int strlimit;
} l_stringmap;

L_EXTERN l_stringmap l_stringmap_new(l_int maxstrlen, const l_strn* str, l_int numofstr, int casesensitive);
L_EXTERN void l_stringmap_set(l_stringmap* self, const l_strn* str, l_int numofstr, int casesensitive);
L_EXTERN void l_stringmap_free(l_stringmap* self);
L_EXTERN const l_stringmap* l_stringmap_space();
L_EXTERN const l_stringmap* l_stringmap_newline();
L_EXTERN const l_stringmap* l_stringmap_blank();
L_EXTERN const l_byte* l_string_match(const l_stringmap* map, l_strt s);
L_EXTERN const l_byte* l_string_matchEx(const l_stringmap* map, l_strt s, l_int* strid, l_int* mlen);
L_EXTERN const l_byte* l_string_matchTimes(const l_stringmap* map, l_int n, l_strt s);
L_EXTERN const l_byte* l_string_matchRepeat(const l_stringmap* map, l_strt s);
L_EXTERN const l_byte* l_string_matchUntil(const l_stringmap* map, l_strt s, l_byte** last_match_start);
L_EXTERN const l_byte* l_string_skipSpaceAndMatchUntil(const l_stringmap* map, l_strt s, l_byte** first_non_space_pos);
L_EXTERN const l_byte* l_string_skipSpaceAndMatch(const l_stringmap* map, l_strt s, l_int* strid, l_int* mlen);
L_EXTERN const l_byte* l_string_trimHead(l_strt s);
L_EXTERN const l_byte* l_string_skipSpaceAndMatchSub(l_strt sub, l_strt s);
L_EXTERN void l_string_match_test();

#endif /* l_core_match_h */

