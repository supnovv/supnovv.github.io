#ifndef lucy_core_shortstr_h
#define lucy_core_shortstr_h
#include "core/base.h"

typedef struct l_shortstr l_shortstr;

l_spec_extern(l_shortstr*)
l_create_shortstr(l_byte size, const void* str, void* (*allocfunc)(l_int));

l_spec_extern(void)
l_destroy_shortstr(l_shortstr* self, void (*freefunc)(void*));

#endif /* lucy_core_shortstr_h */

