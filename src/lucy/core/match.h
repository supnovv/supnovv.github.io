#ifndef lucy_core_match_h
#define lucy_core_match_h
#include "core/base.h"

typedef struct {
  void* t; /* table array, 1 table contains 1 rune, the array size is up to the length of the string */
  l_int size; /* a string map can store strings up to 'maxnumofstr', the size is the length of the longest string */
  l_int maxnumofstr;
} l_stringmap;

l_spec_extern(const l_stringmap*)
l_string_space_map();

l_spec_extern(const l_stringmap*)
l_string_newline_map();

l_spec_extern(const l_stringmap*)
l_string_blank_map();

l_spec_extern(l_stringmap)
l_string_new_map(l_int maxstrlen, const l_strn* str, l_int numofstr, int casesensitive);

l_spec_extern(void)
l_string_set_map(l_stringmap* self, const l_strn* str, l_int numofstr, int casesensitive);

l_spec_extern(void)
l_string_free_map(l_stringmap* self);

l_spec_extern(const l_rune*)
l_string_match(const l_stringmap* map, l_strt s);

l_spec_extern(const l_rune*)
l_string_match_ex(const l_stringmap* map, l_strt s, l_int* strid, l_int* mlen);

l_spec_extern(const l_rune*)
l_string_match_ntimes(const l_stringmap* map, int n, l_strt s);

l_spec_extern(const l_rune*)
l_string_match_repeat(const l_stringmap* map, l_strt s);

l_spec_extern(const l_rune*)
l_string_match_until(const l_stringmap* map, l_strt s, l_rune** last_match_start);

l_spec_extern(const l_rune*)
l_string_skip_space_and_match_until(const l_stringmap* map, l_strt s, l_rune** first_non_space_pos);

l_spec_extern(const l_rune*)
l_string_skip_space_and_match(const l_stringmap* map, l_strt s, l_int* strid, l_int* mlen);

l_spec_extern(const l_rune*)
l_string_trim_head(l_strt s);

l_spec_extern(const l_rune*)
l_string_skip_space_and_match_sub(l_strt sub, l_strt s);

#endif /* lucy_core_match_h */

