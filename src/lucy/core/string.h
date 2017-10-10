#ifndef lucy_string_h
#define lucy_string_h
#include "core/base.h"

#define lstring(s) lp(l_string_cstr(s))

typedef struct {
  void* impl;
} l_string;

typedef struct l_thread l_thread;

L_EXTERN l_string l_string_empty();
L_EXTERN l_string l_string_create(l_int initsize);
L_EXTERN l_string l_string_createFrom(l_strt from);
L_EXTERN l_string l_string_createEx(l_int initsize, l_int maxlimit, l_thread* hint);
L_EXTERN l_string l_string_createFromEx(l_strt from, l_int maxlimit, l_thread* hint);
L_EXTERN void l_string_free(l_string* self, l_thread* hint);
L_EXTERN void l_string_clear(l_string* self);
L_EXTERN l_byte* l_string_cstr(l_string* self);
L_EXTERN l_int l_string_capacity(l_string* self); /* exclude the last byte */
L_EXTERN l_int l_string_remain(l_string* self); /* exclude the last byte */
L_EXTERN l_int l_string_size(l_string* self);
L_EXTERN l_int l_string_limit(l_string* self);
L_EXTERN l_byte* l_string_start(l_string* self);
L_EXTERN l_byte* l_string_end(l_string* self);
L_EXTERN l_strt l_string_strt(l_string* self);
L_EXTERN l_strn l_string_strn(l_string* self);
L_EXTERN int l_string_equal(l_string* self, l_strt s);
L_EXTERN int l_string_isEmpty(l_string* self);
L_EXTERN int l_string_ntEmpty(l_string* self);
L_EXTERN int l_string_ensureCapacity(l_string* self, l_int capacity);
L_EXTERN int l_string_ensureRemain(l_string* self, l_int remainsize);
L_EXTERN int l_string_set(l_string* self, l_strt s);
L_EXTERN int l_string_setLen(l_string* self, const void* s, l_int len);
L_EXTERN int l_string_append(l_string* self, l_strt s);
L_EXTERN int l_string_appendLen(l_string* self, const void* s, l_int len);
L_EXTERN int l_string_appendReversed(l_string* self, l_strt s);
L_EXTERN l_int l_string_appendPossible(l_string* self, l_strt s);
L_EXTERN l_int l_string_appendLenPossible(l_string* self, const void* s, l_int len);
L_EXTERN l_int l_string_appendReversedPossible(l_string* self, l_strt s);
L_EXTERN l_int l_string_parseDec(l_strt s);
L_EXTERN l_int l_string_parseHex(l_strt s);
L_EXTERN int l_string_format_n(l_string* self, const void* fmt, int n, l_value* a);
L_EXTERN int l_string_format_1(l_string* self, const void* fmt, l_value a);
L_EXTERN int l_string_format_2(l_string* self, const void* fmt, l_value a, l_value b);
L_EXTERN int l_string_format_3(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c);
L_EXTERN int l_string_format_4(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d);
L_EXTERN int l_string_format_5(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e);
L_EXTERN int l_string_format_6(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f);
L_EXTERN int l_string_format_7(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g);
L_EXTERN int l_string_format_8(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g, l_value h);
L_EXTERN int l_string_format_9(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i);

L_GLOBAL const l_byte l_char_class_table[256];

L_INLINE int
l_check_is_printable(l_byte ch)
{
  return l_char_class_table[ch];
}

L_INLINE int
l_check_is_dec_digit(l_byte ch)
{
  return l_char_class_table[ch] == 0x3d;
}

L_INLINE int
l_check_is_letter(l_byte ch)
{
  return l_char_class_table[ch] & 0x02;
}

L_INLINE int
l_check_is_upper_letter(l_byte ch)
{
  return (l_char_class_table[ch] & 0x03) == 2;
}

L_INLINE int
l_check_is_lower_letter(l_byte ch)
{
  return (l_char_class_table[ch] & 0x03) == 3;
}

L_INLINE int
l_check_is_hex_digit(l_byte ch)
{
  return l_char_class_table[ch] & 0x04;
}

L_INLINE int
l_check_is_alphanum(l_byte ch)
{
  return l_char_class_table[ch] & 0x08;
}

L_INLINE int
l_check_is_alphanum_underscore(l_byte ch)
{
  return l_char_class_table[ch] & 0x10;
}

L_INLINE int
l_check_is_alphanum_underscore_hyphen(l_byte ch)
{
  return l_char_class_table[ch] & 0x20;
}

#endif /* lucy_string_h */

