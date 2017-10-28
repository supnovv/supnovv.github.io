#ifndef l_core_heap_h
#define l_core_heap_h
#include "core/base.h"

/**
 * minimal and maximal heap - add remove quick, but search slow
 */

typedef struct {
  l_umedit size;
  l_umedit capacity;
  l_int* a; /* array of elements with pointer size */
  int (*less)(void* this_elem_is_less, void* than_this_one);
} l_mmheap;

/* min. heap - pass less function to less, max. heap - pass greater function to less */
l_spec_extern(void)
l_mmheap_init(l_mmheap* self, int (*less)(void*, void*), int initsize);

l_spec_extern(void)
l_mmheap_free(l_mmheap* self);

l_spec_extern(void)
l_mmheap_add(l_mmheap* self, void* elem);

l_spec_extern(void*)
l_mmheap_del(l_mmheap* self, l_umedit i);


void l_mmheap_init(l_mmheap* self, int (*less)(void*, void*), int initsize) {
  self->size = self->capacity = 0;
  self->less = less;
  if (initsize > l_max_rdwr_size) {
    self->a = 0;
    l_loge_1("large %d", ld(initsize));
    return;
  }
  if (initsize < 8) initsize = 8;
  self->a = l_raw_calloc(initsize*sizeof(void*));
  self->capacity = initsize;
}

void l_mmheap_free(l_mmheap* self) {
  if (self->a) {
    l_raw_free(self->a);
    self->a = 0;
  }
}

static l_umedit ll_left(l_umedit n) {
  return n*2 + 1; /* right is left + 1 */
}

static l_umedit ll_parent(l_umedit n) {
  return (n == 0 ? 0 : (n - 1) / 2);
}

static int ll_has_child(l_mmheap* self, l_umedit i) {
  return self->size > ll_left(i);
}

static int ll_less_than(l_mmheap* self, l_umedit i, l_umedit j) {
  return self->less((void*)self->a[i], (void*)self->a[j]);
}

static l_umedit ll_min_child(l_mmheap* self, l_umedit i) {
  if (self->size & 0x01) {
    /* have two children */
    l_umedit left = ll_left(i);
    if (ll_less_than(self, left, left+1)) {
      return left;
    }
    return left+1;
  }
  /* only have left child */
  return ll_left(i);
}

static void ll_add_tail_elem(l_mmheap* self) {
  l_umedit i = self->size;
  while (i != 0) {
    l_umedit pa = ll_parent(i);
    if (ll_less_than(self, i, pa)) {
      l_uint temp = self->a[i];
      self->a[i] = self->a[pa];
      self->a[pa] = temp;
      i = pa;
      continue;
    }
    break;
  }
}

void l_mmheap_add(l_mmheap* self, void* elem) {
  if (self->capacity == 0 || elem == 0) {
    l_loge_s("mmheap_add invalid argument");
    return;
  }
  if (self->size >= self->capacity) {
    self->a = l_raw_realloc(self->a, self->capacity, self->capacity*2);
    self->capacity *= 2;
  }
  self->a[self->size++] = (l_uint)elem;
  ll_add_tail_elem(self);
}

void* l_mmheap_del(l_mmheap* self, l_umedit i) {
  void* elem = 0;
  if (i >= self->size) {
    return 0;
  }

  elem = (void*)self->a[i];
  self->a[i] = self->a[self->size-1];

  while (ll_has_child(self, i)) {
    l_umedit child = ll_min_child(self, i);
    l_uint temp = self->a[i];
    self->a[i] = self->a[child];
    self->a[child] = temp;
    i = child;
  }

  return elem;
}

#endif /* l_core_heap_h */

