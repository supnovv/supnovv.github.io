#ifndef l_table_lib_h
#define l_table_lib_h

/**
 * heap - add/remove quick, search slow
 */

#define CCMMHEAP_MIN_SIZE (8)
struct ccmmheap {
  umedit_int size;
  umedit_int capacity;
  unsign_ptr* a; /* array of elements with pointer size */
  nauty_bool (*less)(void* this_elem_is_less, void* than_this_one);
};

/* min. heap - pass less function to less, max. heap - pass greater function to less */
CORE_API void ccmmheap_init(struct ccmmheap* self, nauty_bool (*less)(void*, void*), int initsize);
CORE_API void ccmmheap_free(struct ccmmheap* self);
CORE_API void ccmmheap_add(struct ccmmheap* self, void* elem);
CORE_API void* ccmmheap_del(struct ccmmheap* self, umedit_int i);

/**
 * hash table - add/remove/search quick, need re-hash when enlarge buffer size
 */

CORE_API nauty_bool ccisprime(umedit_int n);
CORE_API umedit_int ccmidprime(nauty_byte bits);

struct cchashslot {
  void* next;
};

struct cchashtable {
  nauty_byte slotbits;
  ushort_int offsetofnext;
  umedit_int nslot; /* prime number size not near 2^n */
  umedit_int nbucket;
  umedit_int (*getkey)(void*);
  struct cchashslot* slot;
};

CORE_API void cchashtable_init(struct cchashtable* self, nauty_byte sizebits, int offsetofnext, umedit_int (*getkey)(void*));
CORE_API void cchashtable_free(struct cchashtable* self);
CORE_API void cchashtable_foreach(struct cchashtable* self, void (*cb)(void*));
CORE_API void cchashtable_add(struct cchashtable* self, void* elem);
CORE_API void* cchashtable_find(struct cchashtable* self, umedit_int key);
CORE_API void* cchashtable_del(struct cchashtable* self, umedit_int key);

/* table size is enlarged auto */
struct ccbackhash {
  struct cchashtable* cur;
  struct cchashtable* old;
  struct cchashtable a;
  struct cchashtable b;
};

CORE_API void ccbackhash_init(struct ccbackhash* self, nauty_byte initsizebits, int offsetofnext, umedit_int (*getkey)(void*));
CORE_API void ccbackhash_free(struct ccbackhash* self);
CORE_API void ccbackhash_add(struct ccbackhash* self, void* elem);
CORE_API void* ccbackhash_find(struct ccbackhash* self, umedit_int key);
CORE_API void* ccbackhash_del(struct ccbackhash* self, umedit_int key);

#endif /* l_table_lib_h */

