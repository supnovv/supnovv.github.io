#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#define L_CORELIB_IMPL
#include "lucycore.h"

void l_zero_n(void* start, l_int len) {
  if (!start || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("size %d", ld(len));
    return;
  }
  /* void* memset(void* ptr, int value, size_t num); */
  memset(start, 0, (size_t)len);
}

l_byte* l_copy_n(const void* from, l_int len, void* to) {
  if (!from || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("size %d", ld(len));
    return 0;
  }
  if (l_rstr(to) + len <= l_rstr(from) || l_rstr(to) >= l_rstr(from) + len) {
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
  return l_rstr(to) + len;
}

l_byte* l_copy_from(l_strt s, void* to) {
  return l_copy_n(s.start, s.end - s.start, to);
}

static void* l_out_of_memory(l_int size, int init) {
  l_process_exit();
  (void)size;
  (void)init;
  return 0;
}

static l_int l_check_alloc_size(l_int size) {
  if (size <= 0 || size + 8 > l_max_rdwr_size) return 0;
  return (((size - 1) >> 3) + 1) << 3; /* times of eight */
}

void* l_raw_malloc(l_int size) {
  void* p = 0;
  l_int n = l_check_alloc_size(size);
  if (!n) { l_loge_1("large %d", ld(size)); return 0; }
  p = malloc(l_cast(size_t, n));
  if (p) return p; /* not init */
  return l_out_of_memory(n, 0);
}

void* l_raw_calloc(l_int size) {
  void* p = 0;
  l_int n = l_check_alloc_size(size);
  if (!n) { l_loge_1("large %d", ld(size)); return 0; }
  /* void* calloc(size_t num, size_t size); */
  p = calloc(l_cast(size_t, n) >> 3, 8);
  if (p) return p;
  return l_out_of_memory(n, 1);
}

void* l_raw_realloc(void* p, l_int old, l_int newsz) {
  void* temp = 0;
  l_int n = l_check_alloc_size(newsz);
  if (!p || old <= 0 || n == 0) { l_loge_1("size %d", ld(newsz)); return 0; }

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
      l_zero_n(temp + old, n - old);
      return temp;
    }
    if ((temp = l_out_of_memory(n, 0))) {
      l_copy_n(p, old, temp);
      l_zero_n(temp + old, n - old);
      l_raw_free(p);
      return temp;
    }
  } else {
    temp = realloc(p, l_cast(size_t, n));
    if (temp) return temp;
    if ((temp = l_out_of_memory(n, 0))) {
      l_copy_n(p, n, temp);
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

static l_filestream llopenfile(const void* name, const char* mode) {
  l_filestream fs = {0};
  if (!name) { l_loge_s("empty name"); return fs; }
  fs.stream = fopen((const char*)name, mode);
  if (!fs.stream) {
    l_loge_1("fopen %s", lserror(errno));
  }
  return fs;
}

static void llreopenfile(FILE* file, const void* name, const char* mode) {
  if (!name) { l_loge_s("empty name"); return; }
  if (freopen((const char*)name, mode, file) == 0) {
    l_loge_1("freopen %s", lserror(errno));
  }
}

l_filestream l_open_read(const void* name) {
  return llopenfile(name, "rb");
}

l_filestream l_open_write(const void* name) {
  return llopenfile(name, "wb");
}

l_filestream l_open_append(const void* name) {
  return llopenfile(name, "ab");
}

l_filestream l_open_read_write(const void* name) {
  return llopenfile(name, "rb+");
}

l_filestream l_open_read_unbuffered(const void* name) {
  l_filestream fs = llopenfile(name, "rb");
  if (!fs.stream) return fs;
  setbuf((FILE*)fs.stream, 0);
  return fs;
}

l_filestream l_open_write_unbuffered(const void* name) {
  l_filestream fs = llopenfile(name, "wb");
  if (!fs.stream) return fs;
  setbuf((FILE*)fs.stream, 0);
  return fs;
}

l_extern l_filestream l_open_append_unbuffered(const void* name) {
  l_filestream fs = llopenfile(name, "ab");
  if (!fs.stream) return fs;
  setbuf((FILE*)fs.stream, 0);
  return fs;
}

int l_remove_file(const void* name) {
  if (!name) { l_loge_s("empty name"); return L_STATUS_EINVAL; }
  if (remove((const char*)name) != 0) {
    l_loge_1("remove %s", lserror(errno));
    return L_STATUS_ERROR;
  }
  return 0;
}

int l_rename_file(const void* from, const void* to) {
  /* int rename(const char* oldname, const char* newname);
  Changes the name of the file or directory specified by
  oldname to newname. This is an operation performed directly
  on a file; No streams are involved in the operation. If
  oldname and newname specify different paths and this is
  supported by the system, the file is moved to the new
  location. If newname names an existing file, the function
  may either fail or override the existing file, depending on
  the specific system and library implementation. */
  if (!from || !to) { l_loge_s("empty name"); return L_STATUS_EINVAL; }
  if (rename((const char*)from, (const char*)to) != 0) {
    l_loge_1("rename %s", lserror(errno));
    return L_STATUS_ERROR;
  }
  return 0;
}

void l_redirect_stdout(const void* name) {
  llreopenfile(stdout, name, "wb");
}

void l_redirect_stderr(const void* name) {
  llreopenfile(stderr, name, "wb");
}

void l_reditect_stdin(const void* name) {
  llreopenfile(stdin, name, "rb");
}

void l_close_file(l_filestream* self) {
  if (!self->stream) return;
  if (fclose((FILE*)self->stream) != 0) {
    l_loge_1("fclose %s", lserror(errno));
  }
  self->stream = 0;
}

void l_flush_file(l_filestream* self) {
  if (fflush((FILE*)self->stream) != 0) {
    l_loge_1("fflush %s", lserror(errno));
  }
}

void l_rewind_file(l_filestream* self) {
  rewind((FILE*)self->stream);
}

void l_seek_from_begin(l_filestream* self, long offset) {
  if (fseek((FILE*)self->stream, offset, SEEK_SET) != 0) {
    l_loge_1("fseek SET %s", lserror(errno));
  }
}

void l_seek_from_curpos(l_filestream* self, long offset) {
  if (fseek((FILE*)self->stream, offset, SEEK_CUR) != 0) {
    l_loge_1("fseek CUR %s", lserror(errno));
  }
}

void l_clear_file_error(l_filestream* self) {
  clearerr((FILE*)self->stream);
}

l_int l_write_strt_to_file(l_filestream* self, l_strt s) {
  return l_write_file(self, s.start, s.end - s.start);
}

l_int l_write_file(l_filestream* self, const void* s, l_int len) {
  l_int n = 0;
  if (!s || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("invalid %d", ld(len));
    return 0;
  }
  if ((n = (l_int)fwrite(s, 1, (size_t)len, (FILE*)self->stream)) != len) {
    l_loge_1("fwrite %s", lserror(errno));
    if (n <= 0) return 0;
  }
  return n;
}

l_int l_read_file(l_filestream* self, void* out, l_int len) {
  l_int n = 0;
  if (!out || len <= 0 || len > l_max_rdwr_size) {
    l_loge_1("invalid %d", ld(len));
    return 0;
  }
  if ((n = (l_int)fread(out, 1, (size_t)len, (FILE*)self->stream)) != len) {
    if (!feof((FILE*)self->stream)) {
      l_loge_1("fread %s", lserror(errno));
    }
    if (n <= 0) return 0;
  }
  return n;
}

void l_linknode_init(l_linknode* node) {
  node->next = node->prev = node;
}

int l_linknode_is_empty(l_linknode* node) {
  return (node->next == node);
}

void l_linknode_insert_after(l_linknode* node, l_linknode* newnode) {
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

void l_smplnode_init(l_smplnode* node) {
  node->next = node;
}

int l_smplnode_is_empty(l_smplnode* node) {
  return node->next == node;
}

void l_smplnode_insert_after(l_smplnode* node, l_smplnode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
}

l_smplnode* l_smplnode_remove_next(l_smplnode* node) {
  l_smplnode* p = node->next;
  node->next = p->next;
  return p;
}

void l_squeue_init(l_squeue* self) {
  l_smplnode_init(&self->head);
  self->tail = &self->head;
}

void l_squeue_push(l_squeue* self, l_smplnode* newnode) {
  l_smplnode_insert_after(self->tail, newnode);
  self->tail = newnode;
}

void l_squeue_push_queue(l_squeue* self, l_squeue* q) {
  if (l_squeue_is_empty(q)) return;
  self->tail->next = q->head.next;
  self->tail = q->tail;
  self->tail->next = &self->head;
  l_squeue_init(q);
}

int l_squeue_is_empty(l_squeue* self) {
  return (self->head.next == &self->head);
}

l_smplnode* l_squeue_pop(l_squeue* self) {
  l_smplnode* node = 0;
  if (l_squeue_is_empty(self)) {
    return 0;
  }
  node = l_smplnode_remove_next(&self->head);
  if (node == self->tail) {
    self->tail = &self->head;
  }
  return node;
}

void l_dqueue_init(l_dqueue* self) {
  l_linknode_init(&self->head);
}

void l_dqueue_push(l_dqueue* self, l_linknode* newnode) {
  l_linknode_insert_after(&self->head, newnode);
}

void l_dqueue_push_queue(l_dqueue* self, l_dqueue* q) {
  l_linknode* tail = 0;
  if (l_dqueue_is_empty(q)) return;
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

int l_dqueue_is_empty(l_dqueue* self) {
  return l_linknode_is_empty(&self->head);
}

l_linknode* l_dqueue_pop(l_dqueue* self) {
  if (l_dqueue_is_empty(self)) return 0;
  return l_linknode_remove(self->head.prev);
}

void l_priorq_init(l_priorq* self, int (*less)(void*, void*)) {
  l_linknode_init(&self->node);
  self->less = less;
}

int l_priorq_is_empty(l_priorq* self) {
  return l_linknode_is_empty(&self->node);
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
  if (l_priorq_is_empty(self)) {
    return 0;
  }
  head = &(self->node);
  first = head->next;
  head->next = first->next;
  first->next->prev = head;
  return first;
}

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

typedef struct {
  l_smplnode node;
} l_hashslot;

typedef struct {
  l_umedit nslot;
  l_umedit nelem;
  l_umedit seed;
  l_hashslot slot[1];
} l_hashtable;

l_hashtable* l_hashtable_create(l_byte sizebits, l_umedit seed) {
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
  table->seed = seed;

  return table;
}

static l_smplnode* l_hashtable_slot_head_node(l_hashtable* self, l_umedit hash) {
  return &(self->slot[hash & (self->nslot - 1)].node);
}

int l_hashtable_add(l_hashtable* self, l_smplnode* elem, l_umedit hash) {
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

l_smplnode* l_hashtable_find(l_hashtable* self, l_umedit hash, int (*check)(void*, l_smplnode*), void* obj) {
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

l_smplnode* l_hashtable_del(l_hashtable* self, l_umedit hash, int (*check)(void*, l_smplnode*), void* obj) {
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

void l_hashtable_foreach(l_hashtable* self, void (*func)(void*, l_smplnode*), void* obj) {
  l_smplnode* slot = self->slot;
  l_smplnode* send = slot + self->nslot;
  l_smplnode* elem = 0;

  for (; slot < send; ++slot) {
    elem = slot->node.next;
    while (elem) {
      func(obj, elem);
      elem = elem->next;
    }
  }
}

void l_hashtable_clear(l_hashtable* self, void *(freefunc)(l_smplnode*)) {
  l_hashslot* slot = self->slot;
  l_hashslot* send = slot + self->nslot;
  l_smplnode* head = 0;
  l_smplnode* first = 0;

  for (; slot < send; ++slot) {
    head = &slot->node;
    while (head->next) {
      first = head->next;
      head->next = first->next;
      if (freefunc) freefunc(first);
    }
  }
}

void l_hashtable_free(l_hashtable** self, void (*freefunc)(l_smplnode*)) {
  if (*self == 0) return;
  l_hashtable_clear(*self, freefunc);
  l_raw_free(*self);
  *self = 0;
}

typedef struct l_treenode {
  void* data;
  struct l_treenode* next_sibling;
  struct l_treenode* prev_sibling;
  struct l_treenode* first_child;
  struct l_treenode* parent;
} l_treenode;

void l_core_test() {
  char buffer[] = "012345678";
  char* a = buffer;
  l_strt strt = {0};
  l_strt* pstr = &strt;
  l_byte bytes[4] = {1};
#if defined(L_BUILD_DEBUG)
  l_logd_s("L_BUILD_DEBUG true");
#else
  l_logd_s("L_BUILD_DEBUG false");
#endif
  /* struct/array init */
  l_assert(strt.start == 0);
  l_assert(strt.end == 0);
  pstr->start = l_rstr(buffer);
  l_assert(*pstr->start == '0');
  l_assert(*pstr->start == *(pstr->start));
  l_assert(&pstr->start == &(pstr->start));
  l_assert(bytes[0] == 1);
  l_assert(bytes[1] == 0);
  l_assert(bytes[2] == 0);
  l_assert(bytes[3] == 0);
  /* type size */
  l_assert(sizeof(l_byte) == 1);
  l_assert(sizeof(l_sbyte) == 1);
  l_assert(sizeof(l_short) == 2);
  l_assert(sizeof(l_ushort) == 2);
  l_assert(sizeof(l_medit) == 4);
  l_assert(sizeof(l_umedit) == 4);
  l_assert(sizeof(l_long) == 8);
  l_assert(sizeof(l_ulong) == 8);
  l_assert(sizeof(l_uint) == sizeof(void*));
  l_assert(sizeof(l_int) == sizeof(void*));
  l_assert(sizeof(int) >= 4);
  l_assert(sizeof(char) == 1);
  l_assert(sizeof(float) == 4);
  l_assert(sizeof(double) == 8);
  l_assert(sizeof(size_t) >= 4);
  l_assert(sizeof(void*) >= 4);
  l_assert(sizeof(l_float) == 4 || sizeof(l_float) == 8);
  l_assert(sizeof(l_eight) == 8);
  l_assert(sizeof(l_mutex) >= L_MUTEX_SIZE);
  l_assert(sizeof(l_rwlock) >= L_RWLOCK_SIZE);
  l_assert(sizeof(l_condv) >= L_CONDV_SIZE);
  l_assert(sizeof(l_thrkey) >= L_THKEY_SIZE);
  l_assert(sizeof(l_thrid) >= L_THRID_SIZE);
  /* value limit */
  l_assert(l_max_ubyte == 255);
  l_assert(l_max_sbyte == 127);
  l_assert(l_min_sbyte == -127-1);
  l_assert(l_cast(l_byte, l_min_sbyte) == 0x80);
  l_assert(l_cast(l_byte, l_min_sbyte) == 128);
  l_assert(l_max_ushort == 65535);
  l_assert(l_max_short == 32767);
  l_assert(l_min_short == -32767-1);
  l_assert(l_cast(l_ushort, l_min_short) == 32768);
  l_assert(l_cast(l_ushort, l_min_short) == 0x8000);
  l_assert(l_max_umedit == 4294967295);
  l_assert(l_max_medit == 2147483647);
  l_assert(l_min_medit == -2147483647-1);
  l_assert(l_cast(l_umedit, l_min_medit) == 2147483648);
  l_assert(l_cast(l_umedit, l_min_medit) == 0x80000000);
  l_assert(l_max_ulong == 18446744073709551615ull);
  l_assert(l_max_long == 9223372036854775807ull);
  l_assert(l_min_long == -9223372036854775807-1);
  l_assert(l_cast(l_ulong, l_min_long) == 9223372036854775808ull);
  l_assert(l_cast(l_ulong, l_min_long) == 0x8000000000000000ull);
  /* copy test */
  l_copy_n(a, 1, a+1);
  l_assert(a[1] == '0'); a[1] = '1';
  l_copy_n(a+3, 1, a+2);
  l_assert(a[2] == '3'); a[2] = '2';
  l_copy_n(a, 4, a+2);
  l_assert(a[2] == '0');
  l_assert(a[3] == '1');
  l_assert(a[4] == '2');
  l_assert(a[5] == '3');
}
