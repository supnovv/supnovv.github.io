#ifndef lucy_core_table_h
#define lucy_core_table_h
#include "core/base.h"

typedef struct l_hashtable l_hashtable;

L_EXTERN l_hashtable* l_hashtable_create(l_byte sizebits, l_umedit seed);
L_EXTERN int l_hashtable_add(l_hashtable* self, l_smplnode* elem, l_umedit hash);
L_EXTERN l_smplnode* l_hashtable_find(l_hashtable* self, l_umedit hash, int (*check)(void*, l_smplnode*), void* obj);
L_EXTERN l_smplnode* l_hashtable_del(l_hashtable* self, l_umedit hash, int (*check)(void*, l_smplnode*), void* obj);
L_EXTERN void l_hashtable_foreach(l_hashtable* self, void (*func)(void*, l_smplnode*), void* obj);
L_EXTERN void l_hashtable_clear(l_hashtable* self, l_allocfunc func);
L_EXTERN void l_hashtable_free(l_hashtable** self, l_allocfunc func);

#endif /* lucy_core_table_h */

