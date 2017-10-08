#ifndef lucy_string_h
#define lucy_string_h
#include "core/base.h"

#define L_COMMON_BUFHEAD l_smplnode node; l_int bsize;
#define lserror(n) lp(strerror(n))
#define lstring(s) lp(l_string_cstr(s))

typedef struct {
  L_COMMON_BUFHEAD
  l_ushort flags;
  l_ushort nargs;
  l_int size;
  l_int limit;
} l_strbuf;

typedef struct {
  l_strbuf* b;
} l_string;

typedef struct l_thread l_thread;

L_EXTERN l_string l_string_empty();
L_EXTERN l_string l_string_create(l_int initsize);
L_EXTERN l_string l_string_create_from(l_strt from);
L_EXTERN l_string l_string_create_limited(l_int initsize, l_int maxlimit);
L_EXTERN l_string l_string_create_limited_from(l_strt from, l_int maxlimit);
L_EXTERN l_string l_string_thread_create(l_thread* t, l_int initsize);
L_EXTERN l_string l_string_thread_create_from(l_thread* t, l_strt from);
L_EXTERN l_string l_string_thread_create_limited(l_thread* t, l_int initsize, l_int maxlimit);
L_EXTERN l_string l_string_thread_create_limited_from(l_thread* t, l_strt from, l_int maxlimit);
L_EXTERN void l_string_thread_free(l_thread* thread, l_string* self);
L_EXTERN void l_string_free(l_string* self);
L_EXTERN void l_string_clear(l_string* self);
L_EXTERN void l_string_set(l_string* self, l_strt s);  
L_EXTERN int l_string_ensure_capacity(l_string* self, l_int capacity);
L_EXTERN int l_string_ensure_remain(l_string* self, l_int remainsize);
L_EXTERN int l_string_append(l_string* self, l_strt s);
L_EXTERN int l_string_format_impl(l_string* self, const void* fmt, ...);
L_EXTERN int l_string_format_n(l_string* self, const void* fmt, int n, l_value* a);

L_INLINE l_byte*
l_string_cstr(l_string* self)
{
  return (l_byte*)(self->b + 1);
}

L_INLINE l_int
l_string_capacity(l_string* self)
{
  return self->b->bsize - sizeof(l_strbuf);
}

L_INLINE l_int
l_string_size(l_string* self)
{
  return self->b->size;
}

L_INLINE l_byte*
l_string_start(l_string* self)
{
  return l_string_cstr(self);
}

L_INLINE l_byte*
l_string_end(l_string* self)
{
  return l_string_cstr(self) + l_string_size(self);
}

L_INLINE l_strt
l_string_strt(l_string* self)
{
  return l_strt_l(l_string_start(self), self->b->size);
}

L_INLINE int
l_string_is_empty(l_string* self)
{
  return self->b->size == 0;
}

L_INLINE int
l_string_nt_empty(l_string* self)
{
  return self->b->size != 0;
}

L_INLINE l_int
l_string_remain(l_string* self)
{
  return l_string_capacity(self) - l_string_size(self);
}

L_INLINE int
l_string_equal(l_string* self, l_strt s)
{
  return l_strt_equal(l_string_strt(self), s);
}

L_INLINE int
l_string_format_1(l_string* self, const void* fmt, l_value a)
{
  self->b->nargs = 1;
  return l_string_format_impl(self, fmt, a);
}

L_INLINE int
l_string_format_2(l_string* self, const void* fmt, l_value a, l_value b)
{
  self->b->nargs = 2;
  return l_string_format_impl(self, fmt, a, b);
}

L_INLINE int
l_string_format_3(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c)
{
  self->b->nargs = 3;
  return l_string_format_impl(self, fmt, a, b, c);
}

L_INLINE int
l_string_format_4(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d)
{
  self->b->nargs = 4;
  return l_string_format_impl(self, fmt, a, b, c, d);
}

L_INLINE int
l_string_format_5(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e)
{
  self->b->nargs = 5;
  return l_string_format_impl(self, fmt, a, b, c, d, e);
}

L_INLINE int
l_string_format_6(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f)
{
  self->b->nargs = 6;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f);
}

L_INLINE int
l_string_format_7(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g)
{
  self->b->nargs = 7;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g);
}

L_INLINE int
l_string_format_8(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g, l_value h)
{
  self->b->nargs = 8;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g, h);
}

L_INLINE int
l_string_format_9(l_string* self, const void* fmt, l_value a, l_value b,
    l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i)
{
  self->b->nargs = 9;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g, h, i);
}

L_GLOBAL const l_byte l_rune_class_table[256];

L_INLINE int
l_check_is_pritable(l_byte ch)
{
  return l_rune_class_table[ch];
}

L_INLINE int
l_check_is_dec_digit(l_byte ch)
{
  return l_rune_class_table[ch] == 0x3d;
}

L_INLINE int
l_check_is_letter(l_byte ch)
{
  return l_rune_class_table[ch] & 0x02;
}

L_INLINE int
l_check_is_upper_letter(l_byte ch)
{
  return (l_rune_class_table[ch] & 0x03) == 2;
}

L_INLINE int
l_check_is_lower_letter(l_byte ch)
{
  return (l_rune_class_table[ch] & 0x03) == 3;
}

L_INLINE int
l_check_is_hex_digit(l_byte ch)
{
  return l_rune_class_table[ch] & 0x04;
}

L_INLINE int
l_check_is_alphanum(l_byte ch)
{
  return l_rune_class_table[ch] & 0x08;
}

L_INLINE int
l_check_is_alphanum_underscore(l_byte ch)
{
  return l_rune_class_table[ch] & 0x10;
}

L_INLINE int
l_check_is_alphanum_underscore_hyphen(l_byte ch)
{
  return l_rune_class_table[ch] & 0x20;
}

L_EXTERN l_int l_string_parse_dec(l_strt s);
L_EXTERN l_int l_string_parse_hex(l_strt s);

#endif /* lucy_string_h */

