#ifndef l_core_table_h
#define l_core_table_h
#include "core/base.h"

typedef struct l_hashtable l_hashtable;

L_EXTERN l_hashtable* l_hashtable_create(l_byte sizebits);
L_EXTERN int l_hashtable_add(l_hashtable* self, l_smplnode* elem, l_umedit hash);
L_EXTERN l_smplnode* l_hashtable_find(l_hashtable* self, l_umedit hash, int (*check)(void*, l_smplnode*), void* obj);
L_EXTERN l_smplnode* l_hashtable_del(l_hashtable* self, l_umedit hash, int (*check)(void*, l_smplnode*), void* obj);
L_EXTERN void l_hashtable_foreach(l_hashtable* self, void (*func)(void*, l_smplnode*), void* obj);
L_EXTERN void l_hashtable_clear(l_hashtable* self, l_allocfunc func);
L_EXTERN void l_hashtable_free(l_hashtable** self, l_allocfunc func);

class L_EALLOC {};
class L_EINVAL {};

/* following alloc need alloc memory according to the allocobj */
static l_allocfunc l_raw_malloc = l_cstd_mallocfunc;
static l_allocfunc l_coreralloc = l_cstd_rallocfunc;
static l_allocfunc l_coremalloc = l_cstd_callocfunc;
static l_allocfunc l_coremfree = l_cstd_mfreefunc;

template <typename T>
class l_deinit

class l_hashtable {
  l_umedit elems_{0};
  l_umedit slots_;
  l_smplnode* slotarr_;

  l_smplnode* slot_first_element(l_umedit hash) {
    return slotarr_[hash & (slots_ - 1)].next;
  }

public:
  explicit l_hashtable(l_byte sizebits) {
    l_hashtable(sizebits, nullptr, nullptr);
  }

  l_hashtable(l_byte sizebits, l_mallocfunc alloc, void* allocobj) {
    if (sizebits > 30) throw L_EINVAL;
    slots_ = 1 << sizebits;
    if (alloc && allocobj) {
      slotarr_ = (l_smplnode*)l_raw_malloc(allocobj_, sizeof(l_smplnode) * slots_);
    } else {
      slotarr_ = (l_smplnode*)l_
    }
    if (slotarr_ == nullptr) throw L_EALLOC;
    for (l_umedit i = 0; i < slots_; ++i) {
      l_smplnode_init(slotarr_ + i);
    }
  }

  void deinit(l_mfreefunc mfree, void* allocobj) {
    if (slotarr_ == nullptr) return;
    clear();
    mfree(allocobj_, slotarr_);
    slotarr_ = 0;
  }

  void push(l_smplnode* elem, l_umedit hash) {
    l_smplnode* first = slot_first_element(hash);
    elem->next = first->next;
    first->next = elem;
    elems_ += 1;
  }

  l_smplnode* find(l_umedit hash, bool (*equal)(l_smplnode* elem, void* obj), void* obj) {

  }
};

#endif /* l_core_table_h */

