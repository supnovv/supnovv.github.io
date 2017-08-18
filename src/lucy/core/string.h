#ifndef lucy_string_h
#define lucy_string_h

#define L_MKSTR(a) #a
#define L_X_MKSTR(a) L_MKSTR(a)
#define L_MKFLSTR __FILE__ " (" L_X_MKSTR(__LINE__) ") "

#define l_assert(e) ((e) ? l_assert_func_pass("41[D] " L_MKFLSTR, (#e)) : l_assert_func_fail("01[E] " L_MKFLSTR, (#e))) /* 0:assert */
#define l_loge_s(s)                   l_logger_func_s("10[E] " L_MKFLSTR, (s)) /* 1:error */
#define l_loge_1(fmt,a)               l_logger_func_1("11[E] " L_MKFLSTR, (fmt), a)
#define l_loge_n(fmt,n,a)             l_logger_func_n("1n[E] " L_MKFLSTR, (fmt), n,a)
#define l_loge_2(fmt,a,b)             l_logger_func_2("12[E] " L_MKFLSTR, (fmt), a,b)
#define l_loge_3(fmt,a,b,c)           l_logger_func_3("13[E] " L_MKFLSTR, (fmt), a,b,c)
#define l_loge_4(fmt,a,b,c,d)         l_logger_func_4("14[E] " L_MKFLSTR, (fmt), a,b,c,d)
#define l_loge_5(fmt,a,b,c,d,e)       l_logger_func_5("15[E] " L_MKFLSTR, (fmt), a,b,c,d,e)
#define l_loge_6(fmt,a,b,c,d,e,f)     l_logger_func_6("16[E] " L_MKFLSTR, (fmt), a,b,c,d,e,f)
#define l_loge_7(fmt,a,b,c,d,e,f,g)   l_logger_func_7("17[E] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g)
#define l_loge_8(fmt,a,b,c,d,e,f,g,h) l_logger_func_8("18[E] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g,h)
#define l_loge_9(t,a,b,c,d,e,f,g,h,i) l_logger_func_9("19[E] " L_MKFLSTR, (t), a,b,c,d,e,f,g,h,i)
#define l_logw_s(s)                   l_logger_func_s("20[W] " L_MKFLSTR, (s)) /* 2:warning */
#define l_logw_1(fmt,a)               l_logger_func_1("21[W] " L_MKFLSTR, (fmt), a)
#define l_logw_n(fmt,n,a)             l_logger_func_n("2n[W] " L_MKFLSTR, (fmt), n,a)
#define l_logw_2(fmt,a,b)             l_logger_func_2("22[W] " L_MKFLSTR, (fmt), a,b)
#define l_logw_3(fmt,a,b,c)           l_logger_func_3("23[W] " L_MKFLSTR, (fmt), a,b,c)
#define l_logw_4(fmt,a,b,c,d)         l_logger_func_4("24[W] " L_MKFLSTR, (fmt), a,b,c,d)
#define l_logw_5(fmt,a,b,c,d,e)       l_logger_func_5("25[W] " L_MKFLSTR, (fmt), a,b,c,d,e)
#define l_logw_6(fmt,a,b,c,d,e,f)     l_logger_func_6("26[W] " L_MKFLSTR, (fmt), a,b,c,d,e,f)
#define l_logw_7(fmt,a,b,c,d,e,f,g)   l_logger_func_7("27[W] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g)
#define l_logw_8(fmt,a,b,c,d,e,f,g,h) l_logger_func_8("28[W] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g,h)
#define l_logw_9(t,a,b,c,d,e,f,g,h,i) l_logger_func_9("29[W] " L_MKFLSTR, (t), a,b,c,d,e,f,g,h,i)
#define l_logm_s(s)                   l_logger_func_s("30[L] " L_MKFLSTR, (s)) /* 3:main flow */
#define l_logm_1(fmt,a)               l_logger_func_1("31[L] " L_MKFLSTR, (fmt), a)
#define l_logm_n(fmt,n,a)             l_logger_func_n("3n[L] " L_MKFLSTR, (fmt), n,a)
#define l_logm_2(fmt,a,b)             l_logger_func_2("32[L] " L_MKFLSTR, (fmt), a,b)
#define l_logm_3(fmt,a,b,c)           l_logger_func_3("33[L] " L_MKFLSTR, (fmt), a,b,c)
#define l_logm_4(fmt,a,b,c,d)         l_logger_func_4("34[L] " L_MKFLSTR, (fmt), a,b,c,d)
#define l_logm_5(fmt,a,b,c,d,e)       l_logger_func_5("35[L] " L_MKFLSTR, (fmt), a,b,c,d,e)
#define l_logm_6(fmt,a,b,c,d,e,f)     l_logger_func_6("36[L] " L_MKFLSTR, (fmt), a,b,c,d,e,f)
#define l_logm_7(fmt,a,b,c,d,e,f,g)   l_logger_func_7("37[L] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g)
#define l_logm_8(fmt,a,b,c,d,e,f,g,h) l_logger_func_8("38[L] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g,h)
#define l_logm_9(t,a,b,c,d,e,f,g,h,i) l_logger_func_9("39[L] " L_MKFLSTR, (t), a,b,c,d,e,f,g,h,i)
#define l_logd_s(s)                   l_logger_func_s("40[D] " L_MKFLSTR, (s)) /* 4:debug log */
#define l_logd_1(fmt,a)               l_logger_func_1("41[D] " L_MKFLSTR, (fmt), a)
#define l_logd_n(fmt,n,a)             l_logger_func_n("4n[D] " L_MKFLSTR, (fmt), n,a)
#define l_logd_2(fmt,a,b)             l_logger_func_2("42[D] " L_MKFLSTR, (fmt), a,b)
#define l_logd_3(fmt,a,b,c)           l_logger_func_3("43[D] " L_MKFLSTR, (fmt), a,b,c)
#define l_logd_4(fmt,a,b,c,d)         l_logger_func_4("44[D] " L_MKFLSTR, (fmt), a,b,c,d)
#define l_logd_5(fmt,a,b,c,d,e)       l_logger_func_5("45[D] " L_MKFLSTR, (fmt), a,b,c,d,e)
#define l_logd_6(fmt,a,b,c,d,e,f)     l_logger_func_6("46[D] " L_MKFLSTR, (fmt), a,b,c,d,e,f)
#define l_logd_7(fmt,a,b,c,d,e,f,g)   l_logger_func_7("47[D] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g)
#define l_logd_8(fmt,a,b,c,d,e,f,g,h) l_logger_func_8("48[D] " L_MKFLSTR, (fmt), a,b,c,d,e,f,g,h)
#define l_logd_9(t,a,b,c,d,e,f,g,h,i) l_logger_func_9("49[D] " L_MKFLSTR, (t), a,b,c,d,e,f,g,h,i)

typedef union {
  l_long d;
  l_ulong u;
  double f;
  const void* p;
} l_value;

#define ls(s) lp(s)
#define lserror(n) lp(strerror(n))
#define lstring(s) lp(l_string_cstr(s))
#define lc(a) ld(a)
#define lt(a) ld(a)
#define lb(a) lu(a)
#define lo(a) lu(a)
#define lx(a) lu(a)

l_inline l_value lp(const void* p) {
  l_value a; a.p = p; return a;
}

l_inline l_value ld(l_long d) {
  l_value a; a.d = d; return a;
}

l_inline l_value lu(l_ulong u) {
  l_value a; a.u = u; return a;
}

l_inline l_value lf(double f) {
  l_value a; a.f = f; return a;
}

l_extern void l_assert_func_pass(const void* tag, const void* expr);
l_extern void l_assert_func_fail(const void* tag, const void* expr);
l_extern void l_logger_func_impl(const void* tag, const void* fmt, ...);
l_extern void l_set_log_level(int level);
l_extern int l_get_log_level();

l_inline void l_logger_func_s(const void* tag, const void* s) {
  l_logger_func_impl(tag, s, 0);
}

l_inline void l_logger_func_1(const void* tag, const void* s, l_value a) {
  l_logger_func_impl(tag, s, a);
}

l_inline void l_logger_func_2(const void* tag, const void* s, l_value a,
    l_value b) {
  l_logger_func_impl(tag, s, a, b);
}

l_inline void l_logger_func_3(const void* tag, const void* s, l_value a,
    l_value b, l_value c) {
  l_logger_func_impl(tag, s, a, b, c);
}

l_inline void l_logger_func_4(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d) {
  l_logger_func_impl(tag, s, a, b, c, d);
}

l_inline void l_logger_func_5(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e) {
  l_logger_func_impl(tag, s, a, b, c, d, e);
}

l_inline void l_logger_func_6(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f);
}

l_inline void l_logger_func_7(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g);
}

l_inline void l_logger_func_8(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g, h);
}

l_inline void l_logger_func_9(const void* tag, const void* s, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i) {
  l_logger_func_impl(tag, s, a, b, c, d, e, f, g, h, i);
}

l_inline void l_logger_func_n(const void* tag, const void* s, l_int n, const l_value* a) {
  l_logger_func_impl(tag, s, n, a);
}

typedef struct {
  const l_byte* start;
  const l_byte* end;
} l_strt;

typedef struct {
  const l_byte* start;
  l_int n;
} l_strn;

l_inline l_value lstrt(const l_strt* s) {
  return lp(s);
}

l_inline l_value lstrn(const l_strn* s) {
  return lp(s);
}

#define l_literal_strn(s) ((l_strn){l_rstr("" s), sizeof(s) / sizeof(char) - 1})
#define l_literal_strt(s) l_strt_l(l_rstr("" s), sizeof(s) / sizeof(char) - 1)
#define l_empty_strt() ((l_strt){0,0})
#define l_strt_c(s) l_strt_l((s), strlen((char*)(s)))

l_inline l_strt l_strt_e(const void* s, const void* e) {
  return (l_strt){l_rstr(s), l_rstr(e)};
}

l_inline l_strt l_strt_l(const void* s, l_int len) {
  return (l_strt){l_rstr(s), l_rstr(s) + len};
}

l_inline l_strt l_strt_sft(const void* s, l_int from, l_int to) {
  return l_strt_e(l_rstr(s) + from, l_rstr(s) + to);
}

l_inline l_strt l_strn_to_strt(const l_strn* s) {
  return l_strt_l(s->start, s->n);
}

l_inline int l_strt_is_empty(const l_strt* s) {
  return !(s->start != 0 && s->start < s->end);
}

l_extern int l_strt_equal(l_strt l, l_strt r);
l_extern int l_strt_contain(l_strt s, int ch);
l_extern l_byte* l_copy_from(l_strt s, void* to);

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
l_extern l_string l_empty_string();
l_extern l_string l_create_string(l_int initsize);
l_extern l_string l_create_string_from(l_strt from);
l_extern l_string l_create_limited_string(l_int initsize, l_int maxlimit);
l_extern l_string l_create_limited_string_from(l_strt from, l_int maxlimit);
l_extern l_string l_thread_create_string(l_thread* t, l_int initsize);
l_extern l_string l_thread_create_string_from(l_thread* t, l_strt from);
l_extern l_string l_thread_create_limited_string(l_thread* t, l_int initsize, l_int maxlimit);
l_extern l_string l_thread_create_limited_string_from(l_thread* t, l_strt from, l_int maxlimit);
l_extern void l_thread_string_free(l_thread* thread, l_string* self);
l_extern void l_string_free(l_string* self);
l_extern void l_string_clear(l_string* self);
l_extern void l_string_set(l_string* self, l_strt s);
l_extern int l_string_ensure_capacity(l_string* self, l_int capacity);
l_extern int l_string_ensure_remain(l_string* self, l_int remainsize);
l_extern int l_string_append(l_string* self, l_strt s);
l_extern int l_string_format_impl(l_string* self, const void* fmt, ...);
l_extern int l_string_format_n(l_string* self, const void* fmt, int n, l_value* a);

l_inline l_byte* l_string_cstr(l_string* self) {
  return (l_byte*)(self->b + 1);
}

l_inline l_int l_string_capacity(l_string* self) {
  return self->b->bsize - sizeof(l_strbuf);
}

l_inline l_int l_string_size(l_string* self) {
  return self->b->size;
}

l_inline l_byte* l_string_start(l_string* self) {
  return l_string_cstr(self);
}

l_inline l_byte* l_string_end(l_string* self) {
  return l_string_cstr(self) + l_string_size(self);
}

l_inline l_strt l_string_strt(l_string* self) {
  return l_strt_l(l_string_start(self), self->b->size);
}

l_inline int l_string_is_empty(l_string* self) {
  return self->b->size == 0;
}

l_inline int l_string_nt_empty(l_string* self) {
  return self->b->size != 0;
}

l_inline l_int l_string_remain(l_string* self) {
  return l_string_capacity(self) - l_string_size(self);
}

l_inline int l_string_equal(l_string* self, l_strt s) {
  return l_strt_equal(l_string_strt(self), s);
}

l_inline int l_string_format_1(l_string* self, const void* fmt, l_value a) {
  self->b->nargs = 1;
  return l_string_format_impl(self, fmt, a);
}

l_inline int l_string_format_2(l_string* self, const void* fmt, l_value a,
    l_value b) {
  self->b->nargs = 2;
  return l_string_format_impl(self, fmt, a, b);
}

l_inline int l_string_format_3(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c) {
  self->b->nargs = 3;
  return l_string_format_impl(self, fmt, a, b, c);
}

l_inline int l_string_format_4(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d) {
  self->b->nargs = 4;
  return l_string_format_impl(self, fmt, a, b, c, d);
}

l_inline int l_string_format_5(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e) {
  self->b->nargs = 5;
  return l_string_format_impl(self, fmt, a, b, c, d, e);
}

l_inline int l_string_format_6(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f) {
  self->b->nargs = 6;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f);
}

l_inline int l_string_format_7(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g) {
  self->b->nargs = 7;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g);
}

l_inline int l_string_format_8(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h) {
  self->b->nargs = 8;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g, h);
}

l_inline int l_string_format_9(l_string* self, const void* fmt, l_value a,
    l_value b, l_value c, l_value d, l_value e, l_value f, l_value g, l_value h, l_value i) {
  self->b->nargs = 9;
  return l_string_format_impl(self, fmt, a, b, c, d, e, f, g, h, i);
}

typedef struct {
  void* t; /* table array, 1 table contains 1 rune, the array size is up to the length of the string */
  l_int size; /* a string map can store strings up to 'maxnumofstr', the size is the length of the longest string */
  l_int maxnumofstr;
} l_stringmap;

l_extern const l_stringmap* l_string_space_map();
l_extern const l_stringmap* l_string_newline_map();
l_extern const l_stringmap* l_string_blank_map();

l_extern l_stringmap l_string_new_map(l_int maxstrlen, const l_strn* str, l_int numofstr, int casesensitive);
l_extern void l_string_set_map(l_stringmap* self, const l_strn* str, l_int numofstr, int casesensitive);
l_extern void l_string_free_map(l_stringmap* self);

l_extern const l_rune* l_string_match(const l_stringmap* map, l_strt s);
l_extern const l_rune* l_string_match_ex(const l_stringmap* map, l_strt s, l_int* strid, l_int* mlen);
l_extern const l_rune* l_string_match_ntimes(const l_stringmap* map, int n, l_strt s);
l_extern const l_rune* l_string_match_repeat(const l_stringmap* map, l_strt s);
l_extern const l_rune* l_string_match_until(const l_stringmap* map, l_strt s, l_rune** last_match_start);
l_extern const l_rune* l_string_skip_space_and_match_until(const l_stringmap* map, l_strt s, l_rune** first_non_space_pos);
l_extern const l_rune* l_string_skip_space_and_match(const l_stringmap* map, l_strt s, l_int* strid, l_int* mlen);

l_extern const l_rune* l_string_trim_head(l_strt s);
l_extern const l_rune* l_string_skip_space_and_match_sub(l_strt sub, l_strt s);

l_extern const l_rune l_rune_class_table[256];

l_inline int l_check_is_pritable(l_byte ch) {
  return l_rune_class_table[ch];
}

l_inline int l_check_is_dec_digit(l_byte ch) {
  return l_rune_class_table[ch] == 0x3d;
}

l_inline int l_check_is_letter(l_byte ch) {
  return l_rune_class_table[ch] & 0x02;
}

l_inline int l_check_is_upper_letter(l_byte ch) {
  return (l_rune_class_table[ch] & 0x03) == 2;
}

l_inline int l_check_is_lower_letter(l_byte ch) {
  return (l_rune_class_table[ch] & 0x03) == 3;
}

l_inline int l_check_is_hex_digit(l_byte ch) {
  return l_rune_class_table[ch] & 0x04;
}

l_inline int l_check_is_alphanum(l_byte ch) {
  return l_rune_class_table[ch] & 0x08;
}

l_inline int l_check_is_alphanum_underscore(l_byte ch) {
  return l_rune_class_table[ch] & 0x10;
}

l_inline int l_check_is_alphanum_underscore_hyphen(l_byte ch) {
  return l_rune_class_table[ch] & 0x20;
}

l_extern l_int l_string_parse_dec(l_strt s);
l_extern l_int l_string_parse_hex(l_strt s);

#endif /* lucy_string_h */

