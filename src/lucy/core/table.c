#define L_LIBRARY_IMPL
#include "core/table.h"

typedef struct {
  l_smplnode node;
} l_hashslot;

typedef struct l_hashtable {
  l_umedit nslot;
  l_umedit nelem;
  l_hashslot slot[1];
} l_hashtable;

L_EXTERN l_hashtable*
l_hashtable_create(l_byte sizebits)
{
  l_umedit size = 0;
  l_hashtable* table = 0;

  if (sizebits > 30) {
    l_loge_1("slots 2^%d", ld(sizebits));
    return 0;
  }

  size = (1 << sizebits);
  table = (l_hashtable*)l_raw_calloc(sizeof(l_hashtable) + sizeof(l_hashslot) * size);
  table->nslot = size;
  table->nelem = 0;

  return table;
}

static l_smplnode*
l_hashtable_slot_head_node(l_hashtable* self, l_umedit hash)
{
  return &(self->slot[hash & (self->nslot - 1)].node);
}

L_EXTERN int
l_hashtable_add(l_hashtable* self, l_smplnode* elem, l_umedit hash)
{
  l_smplnode* slot = 0;

  if (elem == 0) return false;

  slot = l_hashtable_slot_head_node(self, hash);
  elem->next = slot->next;
  slot->next = elem;
  self->nelem += 1;

  if (self->nelem >= self->nslot) {
    l_logw_2("nelem %d nslot %d", ld(self->nelem), ld(self->nslot));
  }

  return true;
}

L_EXTERN l_smplnode*
l_hashtable_find(l_hashtable* self, l_umedit hash, int (*check)(void*, l_smplnode*), void* obj)
{
  l_smplnode* slot = l_hashtable_slot_head_node(self, hash);
  l_smplnode* elem = slot->next;
  int depth = 0;

  while (elem) {
    depth += 1;
    if (check(obj, elem)) break;
    elem = elem->next;
  }

  if (depth > 3) {
    l_logw_2("hashtable slot depth %d bottom %d", ld(depth), ld(elem == 0));
  }

  return elem;
}

L_EXTERN l_smplnode*
l_hashtable_del(l_hashtable* self, l_umedit hash, int (*check)(void*, l_smplnode*), void* obj)
{
  l_smplnode* elem = l_hashtable_slot_head_node(self, hash);
  l_smplnode* node = elem->next;
  int depth = 0;

  while (node) {
    depth += 1;

    if (check(obj, node)) {
      elem->next = node->next;
      self->nelem -= 1;
      break;
    }

    elem = node;
    node = elem->next;
  }

  if (depth > 3) {
    l_logw_2("hashtable slot depth %d bottom %d", ld(depth), ld(node == 0));
  }

  return node;
}

L_EXTERN void
l_hashtable_foreach(l_hashtable* self, void (*func)(void*, l_smplnode*), void* obj)
{
  l_hashslot* slot = self->slot;
  l_hashslot* send = slot + self->nslot;
  l_smplnode* elem = 0;

  for (; slot < send; ++slot) {
    elem = slot->node.next;
    while (elem) {
      func(obj, elem);
      elem = elem->next;
    }
  }
}

L_EXTERN void
l_hashtable_clear(l_hashtable* self, l_allocfunc func)
{
  l_hashslot* slot = self->slot;
  l_hashslot* send = slot + self->nslot;
  l_smplnode* head = 0;
  l_smplnode* first = 0;

  for (; slot < send; ++slot) {
    head = &slot->node;
    while (head->next) {
      first = head->next;
      head->next = first->next;
      if (func) l_mfree(func, first);
    }
  }
}

L_EXTERN void
l_hashtable_free(l_hashtable** self, l_allocfunc func)
{
  if (*self == 0) return;
  l_hashtable_clear(*self, func);
  l_raw_mfree(*self);
  *self = 0;
}

typedef struct l_treenode {
  void* data;
  struct l_treenode* next_sibling;
  struct l_treenode* prev_sibling;
  struct l_treenode* first_child;
  struct l_treenode* parent;
} l_treenode;

