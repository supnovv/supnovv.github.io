#ifndef l_string_lib_h
#define l_string_lib_h
#include "thatcore.h"
#include "l_thread.h"

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
  l_ushort tid;
  l_byte flags;
  l_byte nargs;
  l_int size;
  l_int limit;
} l_strbuf;

typedef struct {
  l_strbuf* b;
} l_string;

l_inline const l_rune* l_string_cstr(l_string* self) {
  return l_buffer_s(self->b);
}

l_inline l_strt l_string_strt(l_string* self) {
  const l_rune* s = l_buffer_s(self->b);
  return (l_strt){s, self->b->size};
}

l_extern l_string l_string_init(l_strt from);
l_extern l_string l_thread_string_init(l_thread* thread, l_strt from);
l_extern void l_string_free(l_string* self);
l_extern void l_string_clear(l_string* self);
l_extern void l_string_set(l_string* self, l_strt s);
l_extern int l_string_is_empty(l_string* self);
l_extern int l_string_equal(l_string* self, l_strt s);
l_extern int l_string_format_impl(l_string* self, const void* fmt, ...);
l_extern int l_string_format_n_impl(l_string* self, const void* fmt, l_int n, l_value* a);

l_inline int l_string_format_1(l_string* self, const void* fmt, l_valua a) {
  self->b->nargs = 1;
  return l_string_format_impl(self, fmt, a);
}

l_inline int l_string_format_2(l_string* self, const void* fmt, l_valua a,
    l_value b) {
  self->b->nargs = 2;
  return l_string_format_impl(self, fmt, a, b);
}

l_inline int l_string_format_3(l_string* self, const void* fmt, l_valua a,
    l_value b, l_value c) {
  self->b->nargs = 3;
  return l_string_format_impl(self, fmt, a, b, c);
}

l_inline int l_string_format_4(l_string* self, const void* fmt, l_valua a,
    l_value b, l_value c, l_value d) {
  self->b->nargs = 4;
  return l_string_format_impl(self, fmt, a, b, c, d);
}

l_inline int l_string_format_5(l_string* self, const void* fmt, l_valua a,
    l_value b, l_value c, l_value d, l_value e) {
  self->b->nargs = 5;
  return l_string_format_impl(self, fmt, a, b, c, d, e);
}

l_inline int l_string_format_6(l_string* self, const void* fmt, l_valua a,
    l_value b, l_value c, l_value d, l_value e, l_value f) {
  self->b->nargs = 6;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f);
}

l_inline int l_string_format_7(l_string* self, const void* fmt, l_valua a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g) {
  self->b->nargs = 7;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g);
}

l_inline int l_string_format_8(l_string* self, const void* fmt, l_valua a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h) {
  self->b->nargs = 8;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g, h);
}

l_inline int l_string_format_9(l_string* self, const void* fmt, l_valua a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i) {
  self->b->nargs = 9;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g, h, i);
}

l_inline int l_string_format_n(l_string* self, const void* fmt, l_int n, l_value* a) {
  return l_string_format_n_impl(self, fmt, n, a);
}

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

