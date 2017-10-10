#ifndef lucy_core_shortstr_h
#define lucy_core_shortstr_h
#include "core/base.h"

typedef struct l_shortstr l_shortstr;

L_EXTERN l_shortstr* l_shortstr_create(l_byte size, const void* s, l_allocfunc func);
L_EXTERN void l_shortstr_free(l_shortstr* self, l_allocfunc func);

#endif /* lucy_core_shortstr_h */

