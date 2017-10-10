#define L_LIBRARY_IMPL
#include "core/shortstr.h"

typedef struct l_shortstr {
  l_smplnode node;
  struct l_shortstr* next;
  l_byte flags;
  l_byte extra;
  l_byte size;
  l_byte s[1];
} l_shortstr;

L_EXTERN l_umedit
l_string_make_hash(l_strn s, l_umedit seed)
{
  l_int step = (s.len >> 5) + 1;
  l_umedit hash = seed ^ l_cast(l_umedit, s.len);
  for (; s.len >= step; s.len -= step) {
    hash ^= ((hash<<5) + (hash>>2) + s.start[s.len - 1]);
  }
  return hash;
}

static int
l_shortstr_check_func(void* from, l_smplnode* node)
{
  l_strn* s = (l_strn*)from;
  l_shortstr* shortstr = (l_shortstr*)node;
  if (s.len != shortstr->size) return false;
  return memcmp(s.start, shortstr->s, shortstr->size) == 0;
}

L_EXTERN l_shortstr*
l_shortstr_create(l_hashtable* table, l_strn from, l_allocfunc func)
{
  l_umedit hash = 0;
  l_shortstr* shortstr = 0;
  l_smplnode* elem = 0;

  if (l_strn_is_empty(&from) || from.n > 255) return 0;

  hash = l_string_make_hash(from, table->seed);
  if ((elem == l_hashtable_find(table, hash, l_shortstr_check_func, &from)) != 0) {
    return (l_shortstr*)elem;
  }

  shortstr = (l_shortstr*)allocfunc(sizeof(l_shortstr) + from.n);
  if (shortstr == 0) return 0;

  shortstr.next = 0;
  shortstr.flags = shortstr.extra = 0;
  shortstr.size = from.n;
  l_copy_n(from.start, from.n, shortstr.s);
  *(shortstr.s + from.n) = 0;

  l_hashtable_add(table, &shortstr->node, hash);
  return shortstr;
}

L_EXTERN l_byte
l_pnstr_size(l_pnstr* self)
{
  return self->sptr[0];
}

L_EXTERN l_strt
l_pnstr_get_strt(l_pnstr* self)
{
  return l_strt_n(self->sptr + 1, l_pnstr_size(self));
}

L_EXTERN l_strn
l_pnstr_get_strn(l_pnstr* self)
{
  return l_strn_l(self->sptr + 1, l_pnstr_size(self));
}

