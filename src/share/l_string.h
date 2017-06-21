#ifndef l_string_lib_h
#define l_string_lib_h
#include "thatcore.h"

typedef struct {
  const l_byte* start;
  l_int len;
} l_strt;

#define l_strt_c(s) l_strt_l((s), strlen((char*)(s)))

l_inline l_strt l_strt_l(const void* s, l_int len) {
  return ((l_strt){l_rstr(s), len});
}

l_inline l_strt l_strt_e(const void* s, const void* e) {
  return l_strt_l(s, l_rstr(e) - l_rstr(s));
}

#define l_empty_strt() ((l_strt){0,0})
#define l_literal_strt(s) l_strt_l("" s, (sizeof(s)/sizeof(char))-1)
l_extern int l_strt_equal(l_strt lhs, l_strt rhs);
l_extern int l_strt_contain(l_strt s, int ch);

typedef struct {
  L_COMMON_BUFHEAD
  l_thread* belong;
  l_int limit;
  l_int size;
} l_buffer;

l_inline l_byte* l_buffer_s(l_buffer* self) {
  return (l_byte*)(self + 1);
}

l_inline l_int l_buffer_capacity(l_buffer* self) {
  return self->bsize - sizeof(l_buffer);
}

l_extern l_buffer* l_buffer_new(l_thread* thread, l_int initsize, l_int maxlimit);
l_extern void l_buffer_free(l_buffer* buffer);
l_extern int l_buffer_ensure_capacity(l_buffer** self, l_int size);
l_extern int l_buffer_ensure_remain(l_buffer** self, l_int remainsize);

typedef struct {
  l_buffer* b;
} l_string;

l_inline const l_rune* l_string_cstr(l_string* self) {
  return l_buffer_s(self->b);
}

l_inline l_strt l_string_strt(l_string* self) {
  const l_rune* s = l_buffer_s(self->b);
  return (l_strt){s, self->b->size};
}

l_extern l_string l_string_new(l_strt from);
l_extern l_string l_thread_string_new(l_thread* thread, l_strt from);
l_extern void l_string_free(l_string* self);
l_extern void l_string_clear(l_string* self);
l_extern void l_string_set(l_string* self, l_strt s);
l_extern int l_string_is_empty(l_string* self);
l_extern int l_string_equal(l_string* self, l_strt s);

typedef struct {
  l_umedit m; /* string match this rune */
  l_umedit e; /* string ended with this rune */
} l_runeinfo;

typedef struct { /* 4 * 256 * 2 = 2048 (2KB) */
  l_runeinfo a[256];
} l_runetable;

typedef struct {
  l_runetable* t; /* table array, 1 table contains 1 rune, the array size is up to the length of the string */
  int size; /* a string map can store strings up to 'maxnumofstr', the size is the length of the longest string */
  int maxnumofstr;
} l_stringmap;

l_extern const l_stringmap* l_string_space_map();
l_extern const l_stringmap* l_string_newline_map();
l_extern const l_stringmap* l_string_blank_map();

l_extern l_stringmap l_string_new_map(int maxstrlen, const l_rune** str, int numofstr, int casesensitive);
l_extern void l_string_set_map(l_stringmap* self, const l_rune** str, int numofstr, int casesensitive);
l_extern void l_string_free_map(l_stringmap* self);

l_extern const l_rune* l_string_match(const l_stringmap* map, const void* s, int len);
l_extern const l_rune* l_string_match_ex(const l_stringmap* map, const void* s, int len, int* strid, int* mlen);
l_extern const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, const void* s, int len);
l_extern const l_rune* l_string_match_repeat(const l_stringmap* map, const void* s, int len, int* lastmatchfailed);
l_extern const l_rune* l_string_match_until(const l_stringmap* map, const void* s, int len, int* n);
l_extern const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, const void* s, int len, int* n);
l_extern const l_rune* l_string_skip_space_and_match(const l_stringmap* map, const void* s, int len, int* strid, int* mlen);

#endif /* l_string_lib_h */

