#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#define L_CORELIB_IMPL
#include "thatcore.h"
#include "service.h"

void l_exit() {
  exit(EXIT_FAILURE);
}

l_strt l_strt_l(const void* s, l_integer len) {
  l_strt a = {0, 0};
  if (s && len > 0) {
    a.start = l_str(s);
    a.end = a.start + len;
  }
  return a;
}

void l_zero_l(void* start, l_integer len) {
  if (!start || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("invalid %d", ld(len));
    return;
  }
  /* void* memset(void* ptr, int value, size_t num); */
  memset(start, 0, (size_t)len);
}

void l_copy_l(const void* from, l_integer len, void* to) {
  if (!from || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("invalid %d", ld(len));
    return;
  }
  if (l_str(to) + len <= l_str(from) || l_str(to) >= l_str(start) + len) {
    /* void* memcpy(void* destination, const void* source, size_t num);
    To avoid overflows, the size of the arrays pointed to by both the
    destination and source parameters, shall be at least num bytes,
    and should not overlap (for overlapping memory blocks, memmove is
    a safer approach). */
    memcpy(to, from, l_cast(size_t, len));
  } else {
    /* void* memmove(void* destination, const void* source, size_t num);
    Copying takes place as if an intermediate buffer were used,
    allowing the destination and source to overlap. */
    memmove(to, from, l_cast(size_t, len));
  }
}

static void* l_out_of_memory(l_integer size, int init) {
  l_exit();
  (void)size;
  (void)init;
  return 0;
}

static l_integer l_check_alloc_size(l_integer size) {
  if (size > l_max_rdwr_size) return 0;
  if (size <= 0) size = 1;
  return (((size - 1) >> 3) + 1) << 3; /* times of eight */
}

void* l_raw_malloc(l_integer size) {
  void* p = 0;
  l_integer n = l_check_alloc_size(size);
  if (!n) { l_loge_1("large %d", ld(size)); return 0; }
  p = malloc(l_cast(size_t, n));
  if (p) return p; /* not init */
  return l_out_of_memory(n, 0);
}

void* l_raw_calloc(l_integer size) {
  void* p = 0;
  l_integer n = l_check_alloc_size(size);
  if (!n) { l_loge_1("large %d", ld(size)); return 0; }
  /* void* calloc(size_t num, size_t size); */
  p = calloc(l_cast(size_t, n) >> 3, 8);
  if (p) return p;
  return l_out_of_memory(n, 1);
}

void* l_raw_realloc(void* p, l_integer old, l_integer newsz) {
  void* temp = 0;
  l_integer n = l_check_alloc_size(newsz);
  if (!p || old <= 0 || n == 0) { l_loge_1("invalid %d", ld(newsz)); return 0; }

  /** void* realloc(void* buffer, size_t size); **
  Changes the size of the memory block pointed by buffer. The function
  may move the memory block to a new location (its address is returned
  by the function). The content of the memory block is preserved up to
  the lesser of the new and old sizes, even if the block is moved to a
  new location. ***If the new size is larger, the value of the newly
  allocated portion is indeterminate***.
  In case of that buffer is a null pointer, the function behaves like malloc,
  assigning a new block of size bytes and returning a pointer to its beginning.
  If size is zero, the memory previously allocated at buffer is deallocated
  as if a call to free was made, and a null pointer is returned. For c99/c11,
  the return value depends on the particular library implementation, it may
  either be a null pointer or some other location that shall not be dereference.
  If the function fails to allocate the requested block of memory, a null
  pointer is returned, and the memory block pointed to by buffer is not
  deallocated (it is still valid, and with its contents unchanged). */

  if (n > old) {
    temp = realloc(p, l_cast(size_t, n));
    if (temp) { /* the newly allocated portion is indeterminate */
      l_zero_l(temp + old, n - old);
      return temp;
    }
    if ((temp = l_out_of_memory(n, 0))) {
      l_copy_l(p, old, temp);
      l_zero_l(temp + old, n - old);
      l_raw_free(p);
      return temp;
    }
  } else {
    temp = realloc(p, l_cast(size_t, n));
    if (temp) return temp;
    if ((temp = l_out_of_memory(n, 1))) {
      l_copy_l(p, n, temp);
      l_raw_free(p);
      return temp;
    }
  }
  return 0;
}

void l_raw_free(void* p) {
  if (p == 0) return;
  free(p);
}



/** List and queue **/

/**
 * double link list node
 */

void l_linknode_init(l_linknode* node) {
  node->next = node->prev = node;
}

int l_linknode_isempty(l_linknode* node) {
  return (node->next == node);
}

void l_linknode_insertafter(l_linknode* node, l_linknode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
  newnode->prev = node;
  newnode->next->prev = newnode;
}

l_linknode* l_linknode_remove(l_linknode* node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  return node;
}

/**
 * single link list node
 */

void l_smplnode_init(l_smplnode* node) {
  node->next = node;
}

int l_smplnode_isempty(l_smplnode* node) {
  return node->next == node;
}

void l_smplnode_insertafter(l_smplnode* node, l_smplnode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
}

l_smplnode* l_smplnode_removenext(l_smplnode* node) {
  l_smplnode* p = node->next;
  node->next = p->next;
  return p;
}

/**
 * single link queue
 */

void l_squeue_init(l_squeue* self) {
  l_smplnode_init(&self->head);
  self->tail = &self->head;
}

void l_squeue_push(l_squeue* self, l_smplnode* newnode) {
  l_smplnode_insertafter(self->tail, newnode);
  self->tail = newnode;
}

void l_squeue_pushqueue(l_squeue* self, l_squeue* q) {
  if (l_squeue_isempty(q)) return;
  self->tail->next = q->head.next;
  self->tail = q->tail;
  self->tail->next = &self->head;
  l_squeue_init(q);
}

int l_squeue_isempty(l_squeue* self) {
  return (self->head.next == &self->head);
}

l_smplnode* l_squeue_pop(l_squeue* self) {
  l_smplnode* node = 0;
  if (l_squeue_isempty(self)) {
    return 0;
  }
  node = l_smplnode_removenext(&self->head);
  if (node == self->tail) {
    self->tail = &self->head;
  }
  return node;
}

/**
 * double link queue
 */

void l_dqueue_init(l_dqueue* self) {
  l_linknode_init(&self->head);
}

void l_dqueue_push(l_dqueue* self, l_linknode* newnode) {
  l_linknode_insertafter(&self->head, newnode);
}

void l_dqueue_pushqueue(l_dqueue* self, l_dqueue* q) {
  l_linknode* tail = 0;
  if (l_dqueue_isempty(q)) return;
  /* chain self's tail with q's first element */
  tail = self->head.prev;
  tail->next = q->head.next;
  q->head.next->prev = tail;
  /* chain q's tail with self's head */
  tail = q->head.prev;
  tail->next = &self->head;
  self->head.prev = tail;
  /* init q to empty */
  l_dqueue_init(q);
}

int l_dqueue_isempty(l_dqueue* self) {
  return l_linknode_isempty(&self->head);
}

l_linknode* l_dqueue_pop(l_dqueue* self) {
  if (l_dqueue_isempty(self)) return 0;
  return l_linknode_remove(self->head.prev);
}

/**
 * proority double link queue
 */

void l_priorq_init(l_priorq* self, int (*less)(void*, void*)) {
  l_linknode_init(&self->node);
  self->less = less;
}

int l_priorq_isempty(l_priorq* self) {
  return l_linknode_isempty(&self->node);
}

void l_priorq_push(l_priorq* self, l_linknode* elem) {
  l_linknode* head = &(self->node);
  l_linknode* cur = head->next;
  while (cur != head && self->less(cur, elem)) {
    cur = cur->next;
  }
  /* insert elem before current node  or the head node */
  cur->prev->next = elem;
  elem->prev = cur->prev;
  cur->prev = elem;
  elem->next = cur;
}

void l_priorq_remove(l_priorq* self, l_linknode* elem) {
  if (elem == &(self->node)) return;
  l_linknode_remove(elem);
}

l_linknode* l_priorq_pop(l_priorq* self) {
  l_linknode* head;
  l_linknode* first;
  if (l_priorq_isempty(self)) {
    return 0;
  }
  head = &(self->node);
  first = head->next;
  head->next = first->next;
  first->next->prev = head;
  return first;
}

/** Useful structures **/

/**
 * heap - add/remove quick, search slow
 */

void l_mmheap_init(l_mmheap* self, int (*less)(void*, void*), int initsize) {
  self->size = self->capacity = 0;
  self->less = less;
  if (initsize > CCM_RWOP_MAX_SIZE) {
    self->a = 0;
    l_loge("size too large");
    return;
  }
  if (initsize < CCMMHEAP_MIN_SIZE) {
    initsize = CCMMHEAP_MIN_SIZE;
  }
  self->a = l_rawalloc_init(initsize*sizeof(void*));
  self->capacity = initsize;
}

void l_mmheap_free(l_mmheap* self) {
  if (self->a) {
    l_rawfree(self->a);
  }
  l_zeron(self, sizeof(l_mmheap));
}

static umedit_int ll_left(umedit_int n) {
  return n*2 + 1; /* right is left + 1 */
}

static umedit_int ll_parent(umedit_int n) {
  return (n == 0 ? 0 : (n - 1) / 2);
}

static int ll_has_child(l_mmheap* self, umedit_int i) {
  return self->size > ll_left(i);
}

static int ll_less_than(l_mmheap* self, umedit_int i, umedit_int j) {
  return self->less((void*)self->a[i], (void*)self->a[j]);
}

static umedit_int ll_min_child(l_mmheap* self, umedit_int i) {
  if (self->size & 0x01) {
    /* have two children */
    umedit_int left = ll_left(i);
    if (ll_less_than(self, left, left+1)) {
      return left;
    }
    return left+1;
  }
  /* only have left child */
  return ll_left(i);
}

static void ll_add_tail_elem(l_mmheap* self) {
  umedit_int i = self->size;
  while (i != 0) {
    umedit_int pa = ll_parent(i);
    if (ll_less_than(self, i, pa)) {
      unsign_ptr temp = self->a[i];
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
    l_loge("mmheap_add invalid argument");
    return;
  }
  if (self->size >= self->capacity) {
    self->a = l_rawrelloc(self->a, self->capacity, self->capacity*2);
    self->capacity *= 2;
  }
  self->a[self->size++] = (unsign_ptr)elem;
  ll_add_tail_elem(self);
}

void* l_mmheap_del(l_mmheap* self, umedit_int i) {
  void* elem = 0;
  if (i >= self->size) {
    return 0;
  }

  elem = (void*)self->a[i];
  self->a[i] = self->a[self->size-1];

  while (ll_has_child(self, i)) {
    umedit_int child = ll_min_child(self, i);
    unsign_ptr temp = self->a[i];
    self->a[i] = self->a[child];
    self->a[child] = temp;
    i = child;
  }

  return elem;
}

/**
 * hash table - add/remove/search quick, need re-hash when enlarge buffer size
 */

int l_isprime(umedit_int n) {
  l_uinteger i = 0;
  if (n == 2) return true;
  if (n == 1 || (n % 2) == 0) return false;
  for (i = 3; i*i <= n; i += 2) {
    if ((n % i) == 0) return false;
  }
  return true;
}

/* return prime is less than 2^bits and greater then 2^(bits-1) */
umedit_int l_midprime(l_byte bits) {
  umedit_int i = 0, n = 0, mid = 0, m = (1 << bits);
  umedit_int dn = 0, dm = 0; /* distance */
  umedit_int nprime = 0, mprime = 0;
  if (m == 0) return 0; /* max value of bits is 31 */
  if (bits < 3) return bits + 1; /* 1(2^0) 2(2^1) 4(2^2) => 1 2 3 */
  n = (1 << (bits - 1)); /* even number */
  mid = 3 * (n >> 1); /* middle value between n and m, even number */
  for (i = mid - 1; i > n; i -= 2) {
    if (l_isprime(i)) {
      dn = i - n;
      nprime = i;
      break;
    }
  }
  for (i = mid + 1; i < m; i += 2) {
    if (l_isprime(i)) {
      dm = m - i;
      mprime = i;
      break;
    }
  }
  return (dn > dm ? nprime : mprime);
}

void l_hashtable_init(l_hashtable* self, l_byte sizebits, int offsetofnext, umedit_int (*getkey)(void*)) {
  if (sizebits > 30) {
    l_loge("size too large");
    return;
  }
  if (sizebits < 3) {
    sizebits = 3;
  }
  self->slotbits = sizebits;
  self->nslot = l_midprime(sizebits);
  self->slot = (l_hashslot*)l_rawalloc_init(sizeof(l_hashslot)*self->nslot);
  self->nbucket = 0;
  self->offsetofnext = (ushort_int)offsetofnext;
  self->getkey = getkey;
}

void l_hashtable_free(l_hashtable* self) {
  if (self->slot) {
    l_rawfree(self->slot);
    self->slot = 0;
  }
  l_zeron(self, sizeof(l_hashtable));
}

static umedit_int llgethashval(l_hashtable* self, void* elem) {
  return (self->getkey(elem) % self->nslot);
}

static void llsetelemnext(l_hashtable* self, void* elem, void* next) {
  *((void**)((l_byte*)elem + self->offsetofnext)) = next;
}

static void* llgetnextelem(l_hashtable* self, void* elem) {
  return *((void**)((l_byte*)elem + self->offsetofnext));
}

void l_hashtable_add(l_hashtable* self, void* elem) {
  l_hashslot* slot = 0;
  if (elem == 0) return;
  slot = self->slot + llgethashval(self, elem);
  llsetelemnext(self, elem, slot->next);
  slot->next = elem;
  self->nbucket += 1;
}

void l_hashtable_foreach(l_hashtable* self, void (*cb)(void*)) {
  l_hashslot* slot = self->slot;
  l_hashslot* end = slot + self->nslot;
  l_hashslot* elem = 0;
  if (self->slot == 0) return;
  for (; slot < end; ++slot) {
    elem = slot->next;
    while (elem) {
      cb(elem);
      elem = llgetnextelem(self, elem);
    }
  }
}

void* l_hashtable_find(l_hashtable* self, umedit_int key) {
  l_hashslot* slot = 0;
  void* elem = 0;
  slot = self->slot + (key % self->nslot);
  elem = slot->next;
  while (elem != 0) {
    if (self->getkey(elem) == key) {
      return elem;
    }
    elem = llgetnextelem(self, elem);
  }
  return 0;
}

void* l_hashtable_del(l_hashtable* self, umedit_int key) {
  l_hashslot* slot = 0;
  void* prev = 0;
  void* elem = 0;
  slot = self->slot + (key % self->nslot);
  if (slot->next == 0) {
    return 0;
  }
  if (self->getkey(slot->next) == key) {
    elem = slot->next;
    slot->next = llgetnextelem(self, elem);
    self->nbucket -= 1;
    return elem;
  }
  prev = slot->next;
  while ((elem = llgetnextelem(self, prev)) != 0) {
    if (self->getkey(elem) == key) {
      llsetelemnext(self, prev, llgetnextelem(self, elem));
      self->nbucket -= 1;
      return elem;
    }
    prev = elem;
  }
  return 0;
}

void l_backhash_init(l_backhash* self, l_byte initsizebits, int offsetofnext, umedit_int (*getkey)(void*)) {
  l_hashtable_init(&self->a, initsizebits, offsetofnext, getkey);
  self->cur = &(self->a);
  self->b.slot = 0;
  self->old = &(self->b);
}

void l_backhash_free(l_backhash* self) {
  l_hashtable_free(&self->a);
  l_hashtable_free(&self->b);
  self->cur = self->old = 0;
}

static int ll_need_to_enlarge(l_backhash* self) {
  l_hashtable* t = self->cur;
  return (t->nbucket > t->nslot * 2);
}

void l_backhash_add(l_backhash* self, void* elem) {
  if (elem == 0) return;
  if (!ll_need_to_enlarge(self)) {
    l_hashtable_add(self->cur, elem);
  } else {
    l_hashtable oldtable = *(self->old);
    l_hashtable* temp = 0;
    l_hashtable_init(self->old, self->cur->slotbits*2, self->cur->offsetofnext, self->cur->getkey);
    /* switch curtable to new enlarged table */
    temp = self->cur;
    self->cur = self->old;
    self->old = temp;
    /* re-hash previous old table elements to new table */
    if (oldtable.slot && oldtable.nbucket) {
      l_hashslot* slot = oldtable.slot;
      l_hashslot* end = slot + oldtable.nslot;
      l_hashslot* head = 0;
      for (; slot < end; ++slot) {
        head = slot;
        while (head->next) {
          void* elem = head->next;
          head->next = llgetnextelem(&oldtable, elem);
          l_hashtable_add(self->cur, elem);
        }
      }
    }
    if (oldtable.slot) {
      l_hashtable_free(&oldtable);
    }
    l_hashtable_add(self->cur, elem);
  }
}

void* l_backhash_find(l_backhash* self, umedit_int key) {
  void* elem = l_hashtable_find(self->cur, key);
  if (elem == 0 && self->old->slot) {
    return l_hashtable_find(self->old, key);
  }
  return elem;
}

void* l_backhash_del(l_backhash* self, umedit_int key) {
  void* elem = l_hashtable_del(self->cur, key);
  if (elem == 0 && self->old->slot) {
    return l_hashtable_del(self->old, key);
  }
  return elem;
}








/** Core test **/

void l_thattest() {
  char buffer[] = "012345678";
  char* a = buffer;
  l_from from;
  l_heap heap = {0};
  l_heap* pheap = &heap;
  l_byte ainit[4] = {0};
#if defined(CCDEBUG)
  l_logd("CCDEBUG true");
#else
  l_logd("CCDEBUG false");
#endif
  l_assert(heap.start == 0);
  l_assert(heap.beyond == 0);
  pheap->start = (l_byte*)buffer;
  l_assert(*pheap->start == '0');
  l_assert(*pheap->start == *(pheap->start));
  l_assert(&pheap->start == &(pheap->start));
  l_assert(ainit[0] == 0);
  l_assert(ainit[1] == 0);
  l_assert(ainit[2] == 0);
  l_assert(ainit[3] == 0);
  /* basic types */
  l_assert(sizeof(l_byte) == 1);
  l_assert(sizeof(soctet_int) == 1);
  l_assert(sizeof(sshort_int) == 2);
  l_assert(sizeof(ushort_int) == 2);
  l_assert(sizeof(smedit_int) == 4);
  l_assert(sizeof(umedit_int) == 4);
  l_assert(sizeof(l_integer == 8);
  l_assert(sizeof(l_uinteger) == 8);
  l_assert(sizeof(size_t) >= sizeof(void*));
  l_assert(sizeof(l_uinteger) >= sizeof(size_t));
  l_assert(sizeof(unsign_ptr) == sizeof(void*));
  l_assert(sizeof(signed_ptr) == sizeof(void*));
  l_assert(sizeof(int) >= 4);
  l_assert(sizeof(char) == 1);
  l_assert(sizeof(float) == 4);
  l_assert(sizeof(double) == 8);
  l_assert(sizeof(speed_real) == 4 || sizeof(speed_real) == 8);
  l_assert(sizeof(short_real) == 4);
  l_assert(sizeof(right_real) == 8);
  l_assert(sizeof(union l_eight) == 8);
  l_assert(sizeof(l_string) == CCSTRING_SIZEOF);
  l_assert(sizeof(l_mutex) >= CC_MUTEX_BYTES);
  l_assert(sizeof(l_rwlock) >= CC_RWLOCK_BYTES);
  l_assert(sizeof(l_condv) >= CC_CONDV_BYTES);
  l_assert(sizeof(l_thrkey) >= CC_THKEY_BYTES);
  l_assert(sizeof(l_thrid) >= CC_THRID_BYTES);
  /* value ranges */
  l_assert(CCM_UBYT_MAX == 255);
  l_assert(CCM_SBYT_MAX == 127);
  l_assert(CCM_SBYT_MIN == -127-1);
  l_assert((l_byte)CCM_SBYT_MIN == 0x80);
  l_assert((l_byte)CCM_SBYT_MIN == 128);
  l_assert(CCM_USHT_MAX == 65535);
  l_assert(CCM_SSHT_MAX == 32767);
  l_assert(CCM_SSHT_MIN == -32767-1);
  l_assert((ushort_int)CCM_SSHT_MIN == 32768);
  l_assert((ushort_int)CCM_SSHT_MIN == 0x8000);
  l_assert(CCM_UMED_MAX == 4294967295);
  l_assert(CCM_SMED_MAX == 2147483647);
  l_assert(CCM_SMED_MIN == -2147483647-1);
  l_assert((umedit_int)CCM_SMED_MIN == 2147483648);
  l_assert((umedit_int)CCM_SMED_MIN == 0x80000000);
  l_assert(CCM_UINT_MAX == 18446744073709551615ull);
  l_assert(CCM_SINT_MAX == 9223372036854775807ull);
  l_assert(CCM_SINT_MIN == -9223372036854775807-1);
  l_assert((l_uinteger)CCM_SINT_MIN == 9223372036854775808ull);
  l_assert((l_uinteger)CCM_SINT_MIN == 0x8000000000000000ull);
  /* memory operation */
  l_assert(l_multof(0, 0) == 0);
  l_assert(l_multof(0, 4) == 4);
  l_assert(l_multof(4, 0) == 4);
  l_assert(l_multof(4, 1) == 4);
  l_assert(l_multof(4, 2) == 4);
  l_assert(l_multof(4, 3) == 4);
  l_assert(l_multof(4, 4) == 4);
  l_assert(l_multof(4, 5) == 8);
  l_assert(l_multofpow2(0, 0) == 1);
  l_assert(l_multofpow2(0, 4) == 4);
  l_assert(l_multofpow2(2, 0) == 4);
  l_assert(l_multofpow2(2, 1) == 4);
  l_assert(l_multofpow2(2, 2) == 4);
  l_assert(l_multofpow2(2, 3) == 4);
  l_assert(l_multofpow2(2, 4) == 4);
  l_assert(l_multofpow2(2, 5) == 8);
  l_copyfromp(l_setfrom(&from, a, 1), a+1);
  l_assert(*(a+1) == '0'); *(a+1) = '1';
  l_copyfromp(l_setfrom(&from, a+3, 1), a+2);
  l_assert(*(a+2) == '3'); *(a+2) = '2';
  l_copyfromp(l_setfrom(&from, a, 4), a+2);
  l_assert(*(a+2) == '0');
  l_assert(*(a+3) == '1');
  l_assert(*(a+4) == '2');
  l_assert(*(a+5) == '3');
  /* other tests */
  l_assert(l_isprime(0) == false);
  l_assert(l_isprime(1) == false);
  l_assert(l_isprime(2) == true);
  l_assert(l_isprime(3) == true);
  l_assert(l_isprime(4) == false);
  l_assert(l_isprime(5) == true);
  l_assert(l_isprime(CCM_UMED_MAX) == false);
}
