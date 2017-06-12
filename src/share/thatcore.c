#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#define CCLIB_THATCORE
#include "thatcore.h"
#include "service.h"

/** Debugger and logger **/

void cc_assert_func(nauty_bool pass, const char* expr, const char* fileline) {
  if (pass) {
    cc_logger_func("4[D] ", "%sassert pass: %s", fileline, expr);
  } else {
    cc_logger_func("0[E] ", "%sassert fail: %s", fileline, expr);
  }
}

static sright_int ccloglevel = 2;

sright_int cc_logger_func(const char* tag, const void* fmt, ...) {
  sright_int n = 0, level = tag[0] - '0';
  const uoctet_int* p = (const uoctet_int*)fmt;
  int printed = 0;
  if (level > ccloglevel) return 0;
  for (; *p; ++p) {
    if (*p != '%') continue;
    if (*(p+1) == 0) {
      ccloge("cc_logger_func invalid (1) %s", fmt);
      return 0;
    }
    p += 1;
    if (*p == '%' || *p == 's' || *p == 'p') continue;
    ccloge("cc_logger_func invalid (2) %s", fmt);
    return 0;
  }
  if ((n = (sright_int)fprintf(stdout, "%s", tag + 1)) < 0) {
    n = 0;
  }
  if (fmt) {
    va_list args;
    va_start(args, fmt);
    if ((printed = vfprintf(stdout, (const char*)fmt, args)) > 0) {
      n += (sright_int)printed;
    }
    va_end(args);
  }
  if ((printed = fprintf(stdout, CCNEWLINE)) > 0) {
    n += (sright_int)printed;
  }
  return n;
}

void ccsetloglevel(sright_int level) {
  ccloglevel = level;
}

sright_int ccgetloglevel() {
  return ccloglevel;
}

void ccexit() {
  exit(EXIT_FAILURE);
}

/** Memory operation **/

struct ccfrom ccfromrange(const void* start, const void* beyond) {
  struct ccfrom from = {0, 0};
  if (start < beyond) {
    from.start = (const uoctet_int*)start;
    from.beyond = (const uoctet_int*)beyond;
  }
  return from;
}

struct ccfrom ccfromn(const void* start, sright_int bytes) {
  struct ccfrom from = {0, 0};
  if (start && bytes > 0) {
    size_t n = (size_t)bytes;
    if ((uright_int)n != (uright_int)bytes) {
      ccloge("ccfrom size too large %s", ccitos(bytes));
      return from;
    }
    from.start = (const uoctet_int*)start;
    from.beyond = from.start + n;
  }
  return from;
}

struct ccfrom ccfromcstr(const void* cstr) {
  struct ccfrom from = {0, 0};
  if (cstr) {
    from.start = (const nauty_byte*)cstr;
    from.beyond = from.start + strlen(cstr);
  }
  return from;
}



struct ccfrom* ccsetfrom(struct ccfrom* self, const void* start, sright_int bytes) {
  *self = ccfromn(start, bytes);
  return self;
}

struct ccdest ccdest(void* start, sright_int bytes) {
  struct ccdest dest = {0, 0};
  if (start && bytes > 0) {
    size_t n = (size_t)bytes;
    if ((uright_int)n != (uright_int)bytes) {
      ccloge("ccdest size too large %s", ccitos(bytes));
      return dest;
    }
    dest.start = (uoctet_int*)start;
    dest.beyond = dest.start + n;
  }
  return dest;
}

struct ccdest ccdestrange(void* start, void* beyond) {
  struct ccdest dest = {0, 0};
  if (start < beyond) {
    dest.start = (uoctet_int*)start;
    dest.beyond = (uoctet_int*)beyond;
  }
  return dest;
}

const struct ccdest* ccdestheap(const struct ccheap* heap) {
  return (const struct ccdest*)heap;
}

static uright_int ccmultof(uright_int n, uright_int orig) {
  /* ((orig + n - 1) / n) * n = ((orig - 1) / n + 1) * n */
  return ((n == 0 || orig == 0) ? (n | orig) : ((orig - 1) / n + 1) * n);
}

static uright_int ccmultofpow2(uoctet_int bits, uright_int orig) {
  return (orig == 0 ? (1u << bits) : ((((orig - 1) >> bits) + 1) << bits));
}

static size_t llgetallocsize(sright_int bytes) {
  if (bytes > CCM_RWOP_MAX_SIZE) {
    return 0;
  }
  if (bytes <= 0) bytes = 1;
  return (size_t)ccmultofpow2(3, bytes);
}

static void* lloutofmemory(size_t bytes) {
  ccexit();
  (void)bytes;
  return 0;
}

static void* llrawalloc(size_t bytes) { /* bytes should not be zero */
  void* buffer;
  if ((buffer = malloc(bytes)) == 0) {
    ccloge("malloc failed %s", ccutos((uright_int)bytes));
    buffer = lloutofmemory(bytes);
  }
  return buffer;
}

static void* llrawrelloc(void* buffer, size_t bytes) { /* both buffer and bytes should not be zero */
  /** Reallocate memory block **
  void* realloc(void* buffer, size_t size);
  Changes the size of the memory block pointed by buffer. The function
  may move the memory block to a new location (its address is returned
  by the function). The content of the memory block is preserved up to
  the lesser of the new and old sizes, even if the block is moved to a
  new location. ******If the new size is larger, the value of the newly
  allocated portion is indeterminate******.
  In case of that buffer is a null pointer, the function behaves like malloc,
  assigning a new block of size bytes and returning a pointer to its beginning.
  If size is zero, the memory previously allocated at buffer is deallocated
  as if a call to free was made, and a null pointer is returned. For c99/c11,
  the return value depends on the particular library implementation, it may
  either be a null pointer or some other location that shall not be dereference.
  If the function fails to allocate the requested block of memory, a null
  pointer is returned, and the memory block pointed to by buffer is not
  deallocated (it is still valid, and with its contents unchanged). */
  void* tempbuffer = realloc(buffer, bytes);
  if (tempbuffer == 0) {
    ccloge("realloc failed %s", ccutos(bytes));
    if ((tempbuffer = lloutofmemory(bytes))) {
      struct ccfrom from = ccfromrange(buffer, ((uoctet_int*)buffer) + bytes);
      cccopyfromp(&from, tempbuffer);
      ccrawfree(buffer);
    }
  }
  return tempbuffer;
}

void* ccrawalloc(sright_int size) {
  size_t bytes = llgetallocsize(size);
  if (bytes == 0) {
    ccloge("ccrawalloc too large %s", ccitos(size));
    return 0;
  }
  /* returned buffer is not initialized */
  return llrawalloc(bytes);
}

void* ccrawalloc_init(sright_int size) {
  size_t bytes = llgetallocsize(size);
  void* buffer = 0;
  if (bytes == 0) {
    ccloge("ccrawalloc too large %s", ccitos(size));
    return 0;
  }
  buffer = llrawalloc(bytes);
  cczeron(buffer, bytes);
  return buffer;
}

void* ccrawrelloc(void* buffer, sright_int oldsize, sright_int newsize) {
  size_t bytes;

  if (!buffer || oldsize < 0 || (bytes = llgetallocsize(newsize)) == 0) {
    ccloge("ccrawrelloc invalid argument %s", ccitos(newsize));
    return 0;
  }

  if ((uright_int)bytes > (uright_int)oldsize) {
    nauty_byte* p = (nauty_byte*)llrawrelloc(buffer, bytes);
    cczero(p+oldsize, p+bytes);
    return p;
  }
  return llrawrelloc(buffer, bytes);
}

void ccrawfree(void* buffer) {
  if (buffer == 0) return;
  free(buffer);
}

struct ccheap ccheap_allocrawbuffer(sright_int size) {
  struct ccheap heap = {0, 0};
  size_t bytes;
  if ((bytes = llgetallocsize(size)) == 0) {
    ccloge("allocrawbuffer too large %s", ccitos(size));
    return heap;
  }
  /* returned buffer is not initialized */
  if ((heap.start = (uoctet_int*)llrawalloc(bytes))) {
    heap.beyond = heap.start + bytes;
  }
  return heap;
}

struct ccheap ccheap_alloc(sright_int size) {
  struct ccheap heap = ccheap_allocrawbuffer(size);
  cczero(heap.start, heap.beyond);
  return heap;
}

struct ccheap ccheap_allocfrom(sright_int size, struct ccfrom from) {
  struct ccheap heap = ccheap_allocrawbuffer(size);
  uoctet_int* beyond = heap.start + cccopyfromptodest(&from, ccdestheap(&heap));
  if (beyond < heap.beyond) {
    cczero(beyond, heap.beyond);
  }
  return heap;
}

void ccheap_relloc(struct ccheap* self, sright_int newsize) {
  size_t n;
  void* buffer;

  if (self->start == 0) {
    *self = ccheap_alloc(newsize);
    return;
  }

  if ((n = llgetallocsize(newsize)) == 0) {
    ccloge("ccheap_relloc too large %s", ccitos(newsize));
    ccheap_free(self);
    return;
  }

  /* already large enough */
  if (self->start + n <= self->beyond) {
    self->beyond = self->start + n;
    return;
  }

  /* request size is larger */
  if ((buffer = llrawrelloc(self->start, n)) == 0) {
    ccloge("ccheap_relloc failed %s", ccutos(n));
    return;
  }
  cczero(self->beyond, buffer + n);
  self->start = (uoctet_int*)buffer;
  self->beyond = self->start + n;
}

void ccheap_free(struct ccheap* self) {
  if (self->start == 0) return;
  free(self->start);
  self->start = 0;
  self->beyond = 0;
}

sright_int cczeron(void* start, sright_int bytes) {
  if (start && bytes > 0) {
    size_t n = (size_t)bytes;
    memset(start, 0, n);
    if ((uright_int)n != (uright_int)bytes) {
      ccloge("cczero size too large %s", ccitos(bytes));
    }
    return (sright_int)n;
  }
  return 0;
}

ccnauty_int cczerol(void* start, ccnauty_int len) {
  if (!start || len <= 0 || len > CCM_RWOP_MAX_SIZE) {
    ccloge("invalid argument %s", ccitos(len));
    return 0;
  }
  memset(start, 0, (size_t)len);
  return len;
}

sright_int cczero(void* start, const void* beyond) {
  if (start < beyond) {
    size_t n = ((uoctet_int*)beyond) - ((uoctet_int*)start);
    memset(start, 0, n);
    if ((uright_int)((sright_int)n) != (uright_int)n) {
      ccloge("cczero size too large %s", ccutos(n));
    }
    return (sright_int)n;
  }
  return 0;
}

static sright_int llcopy(const uoctet_int* start, size_t n, nauty_byte* dest) {
  if (dest + n <= start || dest >= start + n) {
    memcpy(dest, start, n);
  } else {
    memmove(dest, start, n);
  }
  if ((uright_int)((sright_int)n) != (uright_int)n) {
    ccloge("llcopy size too large %s", ccutos(n));
  }
  return (sright_int)n;
}

sright_int cccopyfromptodest(const struct ccfrom* from, const struct ccdest* dest) {
  size_t nfrom, ndest;
  if (from->start >= from->beyond) return 0;
  if (dest->start >= dest->beyond) return 0;
  nfrom = from->beyond - from->start;
  ndest = dest->beyond - dest->start;
  return llcopy(from->start, (ndest < nfrom ? ndest : nfrom), dest->start);
}

sright_int cccopyfrom(struct ccfrom from, void* dest) {
  return cccopyfromp(&from, dest);
}

sright_int cccopyfromp(const struct ccfrom* from, void* dest) {
  if (from->start < from->beyond && dest) {
    return llcopy(from->start, from->beyond - from->start, (nauty_byte*)dest);
  }
  return 0;
}

static sright_int llrcopy(const uoctet_int* start, size_t n, uoctet_int* dest) {
  if (dest + n <= start || dest >= start + n) {
    const uoctet_int* beyond = start + n;
    while (beyond-- > start) {
      *dest++ = *beyond;
    }
  } else {
    uoctet_int* last = dest + n - 1;
    memmove(dest, start, n); /* copy content first */
    while (dest < last) { /* and do reverse */
      uoctet_int ch = *dest;
      *dest++ = *last;
      *last-- = ch;
    }
  }
  if ((uright_int)((sright_int)n) != (uright_int)n) {
    ccloge("llrcopy size too large %s", ccutos(n));
  }
  return (sright_int)n;
}

sright_int ccrcopyfromp(const struct ccfrom* from, void* dest) {
  if (from->start < from->beyond && dest) {
    return llrcopy(from->start, from->beyond - from->start, (uoctet_int*)dest);
  }
  return 0;
}

sright_int ccrcopyfromptodest(const struct ccfrom* from, const struct ccdest* dest) {
  size_t nfrom, ndest;
  if (from->start >= from->beyond) return 0;
  if (dest->start >= dest->beyond) return 0;
  nfrom = from->beyond - from->start;
  ndest = dest->beyond - dest->start;
  return llrcopy(from->start, (ndest < nfrom ? ndest : nfrom), dest->start);
}

/** String **/

#define LLSMASK 0x0700
#define LLNSIGN 0x0100
#define CCPSIGN 0x0200
#define CCESIGN 0x0400
#define CCZFILL 0x0800
#define CCJUSTL 0x1000
#define CCFORCE 0x2000
#define CCTOHEX 0x4000

static const uoctet_int cchexchars[] = "0123456789abcdef";

static sright_int llutos(uright_int n, sright_int minlen, void* to) {
  /* 64-bit unsigned int max value 18446744073709552046
     it is 20 characters, can be contained in struct ccstring staticly */
  uoctet_int a[CCSTRING_STATIC_CHARS+1] = {0};
  uoctet_int* dest = (uoctet_int*)to;
  uright_int flag = (minlen & 0x7f00);
  sright_int i = 0, end = 0;
  if (dest == 0) return 0;
  if (flag & CCTOHEX) {
    a[i++] = cchexchars[n & 0x0f];
    while ((n >>= 4)) {
      a[i++] = cchexchars[n & 0x0f];
    }
  } else {
    a[i++] = (n % 10) + '0';
    while ((n /= 10)) {
      a[i++] = (n % 10) + '0';
    }
  }
  minlen = (minlen & 0xff);
  if (minlen > CCSTRING_STATIC_CHARS) minlen = CCSTRING_STATIC_CHARS;
  if (flag & LLSMASK) {
    minlen = (flag & (CCTOHEX | CCFORCE)) ? minlen - 3 : minlen - 1;
  }
  if (flag & CCZFILL) {
    while (i < minlen) a[i++] = '0';
  }
  if (flag & (CCTOHEX | CCFORCE)) {
    a[i++] = 'x';
    a[i++] = '0';
  }
  if (flag & LLSMASK) {
    a[i++] = (flag & LLNSIGN) ? '-' : ((flag & CCPSIGN) ? '+' : ' ');
  }
  if (flag & CCJUSTL) {
    if (i < minlen && (flag & CCZFILL) == 0) {
      end = minlen;
      while (i > 0) a[--end] = a[--i];
      while (i < end) a[i++] = ' ';
      i = minlen;
    }
  } else { /* default is right justification */
    if ((flag & CCZFILL) == 0) {
      while (i < minlen) a[i++] = ' ';
    }
  }
  CCDZONE(
    ccassert(i <= CCSTRING_STATIC_CHARS);
  )
  while(i > 0) *dest++ = a[--i];
  *dest = 0;
  return dest - (uoctet_int*)to;
}

nauty_bool ccstrequal(struct ccfrom a, struct ccfrom b) {
  if (a.beyond - a.start != b.beyond - b.start) return false;
  while (a.start < a.beyond) {
    if (*(a.start++) != *(b.start++)) return false;
  }
  return true;
}

sright_int ccstrgetlen(const struct ccstring* self) {
  return (self->flag == 0xFF ? self->len : self->flag);
}

nauty_bool ccstrisempty(struct ccstring* self) {
  return ccstrgetlen(self) == 0;
}

const char* ccstrgetcstr(const struct ccstring* self) {
  return (self->flag == 0xFF ? (const char*)self->heap.start : (const char*)self);
}

struct ccfrom ccstrgetfrom(const struct ccstring* s) {
  return ccfromn(ccstrgetcstr(s), ccstrgetlen(s));
}

ccfrom ccstring_getfrom(const ccstring* s) {
  return ccfromn(ccstrgetcstr(s), ccstrgetlen(s));
}

sright_int ccstrcapacity(const struct ccstring* self) {
  if (self->flag == 0xFF) {
    const struct ccheap* heap = &self->heap;
    return (heap->start < heap->beyond) ? (heap->beyond - heap->start - 1) : 0;
  }
  return CCSTRING_STATIC_CHARS;
}

static struct ccstring* llstrsetlen(struct ccstring* self, sright_int len) {
  (self->flag == 0xFF) ? (self->len = len) : (self->flag = (uoctet_int)len);
  return self;
}

static nauty_char* llstrenlarge(struct ccstring* self, sright_int bytes) {
  if (ccstrcapacity(self) < bytes) {
    sright_int len = 2 * ccstrgetlen(self); /* 1-byte for zero terminal */
    bytes = len >= (bytes+1) ? len : (bytes+1);
    if (self->flag == 0xFF) {
      ccheap_relloc(&self->heap, bytes);
      return self->heap.start;
    }
    if ((self->heap = ccheap_allocfrom(bytes, ccstrgetfrom(self))).start == 0) {
      return 0;
    }
    self->len = self->flag;
    self->flag = 0xFF;
    return self->heap.start;
  }
  return (uoctet_int*)ccstrgetcstr(self);
}

struct ccstring ccemptystr() {
  return (struct ccstring){{0}, 0, {0}, 0};
}

struct ccstring ccstrfromu(uright_int a) {
  return ccstrfromuf(a, 0);
}

struct ccstring ccstrfromuf(uright_int a, sright_int fmt) {
  struct ccstring s = ccemptystr();
  return *llstrsetlen(&s, llutos(a, fmt, &s));
}

struct ccstring ccstrfromi(sright_int a) {
  return ccstrfromif(a, 0);
}

struct ccstring ccstrfromif(sright_int a, sright_int fmt) {
  return ccstrfromuf(a < 0 ? (-a) : a, a < 0 ? (LLNSIGN | fmt) : fmt);
}

struct ccstring* ccstrsetuf(struct ccstring* self, uright_int a, sright_int fmt) {
  return llstrsetlen(self, llutos(a, fmt, llstrenlarge(self, CCSTRING_STATIC_CHARS)));
}

struct ccstring* ccstrsetu(struct ccstring* self, uright_int a) {
  return ccstrsetuf(self, a, 0);
}

struct ccstring* ccstrsetif(struct ccstring* self, sright_int a, sright_int fmt) {
  return llstrsetlen(self, llutos(a < 0 ? (-a) : a, (a < 0 ? (fmt | LLNSIGN) : fmt), llstrenlarge(self, CCSTRING_STATIC_CHARS)));
}

struct ccstring* ccstrseti(struct ccstring* self, sright_int a) {
  return ccstrsetif(self, a, 0);
}

void ccstrfree(struct ccstring* self) {
  if (self->flag == 0xFF) {
    ccheap_free(&self->heap);
    self->len = 0;
  }
  self->flag = 0;
}

const char* ccutos(uright_int a) {
  return ccstrgetcstr(ccstrsetu(ccthread_getdefstr(), a));
}

const char* ccitos(sright_int a) {
  return ccstrgetcstr(ccstrseti(ccthread_getdefstr(), a));
}


void ccstring_free(struct ccstring* self) {
  if (self->flag == 0xFF) {
    ccheap_free(&self->heap);
    self->len = 0;
  }
  self->flag = 0;
}

const char* ccstring_getcstr(const struct ccstring* self) {
  return (self->flag == 0xFF ? (const char*)self->heap.start : (const char*)self);
}

sright_int ccstring_getlen(const struct ccstring* self) {
  return (self->flag == 0xFF ? self->len : self->flag);
}

nauty_bool ccstring_equalcstr(const struct ccstring* self, const void* cstr) {
  return ccstrequal(ccfromn(ccstring_getcstr(self), ccstring_getlen(self)), ccfromcstr(cstr));
}

struct ccstring ccstring_emptystr() {
  return (struct ccstring){{0}, 0, {0}, 0};
}

void ccstring_setempty(struct ccstring* self) {
  ccstring_free(self);
  *self = ccstring_emptystr();
}

void ccstring_setcstr(struct ccstring* self, const void* cstr) {
  sright_int n = strlen((const char*)cstr);
  nauty_char* p = llstrenlarge(self, n);
  llstrsetlen(self, cccopyfrom(ccfromn(cstr, n), p));
}

nauty_bool ccstring_contain(struct ccfrom s, nauty_char ch) {
  return ccstring_containp(&s, ch);
}

nauty_bool ccstring_containp(const struct ccfrom* s, nauty_char ch) {
  const nauty_byte* start = s->start;
  if (start >= s->beyond) return false;
  while (start < s->beyond) {
    if (*start++ == (nauty_byte)ch) {
      return true;
    }
  }
  return false;
}

/** List and queue **/

/**
 * double link list node
 */

void cclinknode_init(struct cclinknode* node) {
  node->next = node->prev = node;
}

nauty_bool cclinknode_isempty(struct cclinknode* node) {
  return (node->next == node);
}

void cclinknode_insertafter(struct cclinknode* node, struct cclinknode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
  newnode->prev = node;
  newnode->next->prev = newnode;
}

struct cclinknode* cclinknode_remove(struct cclinknode* node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  return node;
}

/**
 * single link list node
 */

void ccsmplnode_init(struct ccsmplnode* node) {
  node->next = node;
}

nauty_bool ccsmplnode_isempty(struct ccsmplnode* node) {
  return node->next == node;
}

void ccsmplnode_insertafter(struct ccsmplnode* node, struct ccsmplnode* newnode) {
  newnode->next = node->next;
  node->next = newnode;
}

struct ccsmplnode* ccsmplnode_removenext(struct ccsmplnode* node) {
  struct ccsmplnode* p = node->next;
  node->next = p->next;
  return p;
}

/**
 * single link queue
 */

void ccsqueue_init(struct ccsqueue* self) {
  ccsmplnode_init(&self->head);
  self->tail = &self->head;
}

void ccsqueue_push(struct ccsqueue* self, struct ccsmplnode* newnode) {
  ccsmplnode_insertafter(self->tail, newnode);
  self->tail = newnode;
}

void ccsqueue_pushqueue(struct ccsqueue* self, struct ccsqueue* q) {
  if (ccsqueue_isempty(q)) return;
  self->tail->next = q->head.next;
  self->tail = q->tail;
  self->tail->next = &self->head;
  ccsqueue_init(q);
}

nauty_bool ccsqueue_isempty(struct ccsqueue* self) {
  return (self->head.next == &self->head);
}

struct ccsmplnode* ccsqueue_pop(struct ccsqueue* self) {
  struct ccsmplnode* node = 0;
  if (ccsqueue_isempty(self)) {
    return 0;
  }
  node = ccsmplnode_removenext(&self->head);
  if (node == self->tail) {
    self->tail = &self->head;
  }
  return node;
}

/**
 * double link queue
 */

void ccdqueue_init(struct ccdqueue* self) {
  cclinknode_init(&self->head);
}

void ccdqueue_push(struct ccdqueue* self, struct cclinknode* newnode) {
  cclinknode_insertafter(&self->head, newnode);
}

void ccdqueue_pushqueue(struct ccdqueue* self, struct ccdqueue* q) {
  struct cclinknode* tail = 0;
  if (ccdqueue_isempty(q)) return;
  /* chain self's tail with q's first element */
  tail = self->head.prev;
  tail->next = q->head.next;
  q->head.next->prev = tail;
  /* chain q's tail with self's head */
  tail = q->head.prev;
  tail->next = &self->head;
  self->head.prev = tail;
  /* init q to empty */
  ccdqueue_init(q);
}

nauty_bool ccdqueue_isempty(struct ccdqueue* self) {
  return cclinknode_isempty(&self->head);
}

struct cclinknode* ccdqueue_pop(struct ccdqueue* self) {
  if (ccdqueue_isempty(self)) return 0;
  return cclinknode_remove(self->head.prev);
}

/**
 * proority double link queue
 */

void ccpriorq_init(struct ccpriorq* self, nauty_bool (*less)(void*, void*)) {
  cclinknode_init(&self->node);
  self->less = less;
}

nauty_bool ccpriorq_isempty(struct ccpriorq* self) {
  return cclinknode_isempty(&self->node);
}

void ccpriorq_push(struct ccpriorq* self, struct cclinknode* elem) {
  struct cclinknode* head = &(self->node);
  struct cclinknode* cur = head->next;
  while (cur != head && self->less(cur, elem)) {
    cur = cur->next;
  }
  /* insert elem before current node  or the head node */
  cur->prev->next = elem;
  elem->prev = cur->prev;
  cur->prev = elem;
  elem->next = cur;
}

void ccpriorq_remove(struct ccpriorq* self, struct cclinknode* elem) {
  if (elem == &(self->node)) return;
  cclinknode_remove(elem);
}

struct cclinknode* ccpriorq_pop(struct ccpriorq* self) {
  struct cclinknode* head;
  struct cclinknode* first;
  if (ccpriorq_isempty(self)) {
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

void ccmmheap_init(struct ccmmheap* self, nauty_bool (*less)(void*, void*), int initsize) {
  self->size = self->capacity = 0;
  self->less = less;
  if (initsize > CCM_RWOP_MAX_SIZE) {
    self->a = 0;
    ccloge("size too large");
    return;
  }
  if (initsize < CCMMHEAP_MIN_SIZE) {
    initsize = CCMMHEAP_MIN_SIZE;
  }
  self->a = ccrawalloc_init(initsize*sizeof(void*));
  self->capacity = initsize;
}

void ccmmheap_free(struct ccmmheap* self) {
  if (self->a) {
    ccrawfree(self->a);
  }
  cczeron(self, sizeof(struct ccmmheap));
}

static umedit_int ll_left(umedit_int n) {
  return n*2 + 1; /* right is left + 1 */
}

static umedit_int ll_parent(umedit_int n) {
  return (n == 0 ? 0 : (n - 1) / 2);
}

static nauty_bool ll_has_child(struct ccmmheap* self, umedit_int i) {
  return self->size > ll_left(i);
}

static nauty_bool ll_less_than(struct ccmmheap* self, umedit_int i, umedit_int j) {
  return self->less((void*)self->a[i], (void*)self->a[j]);
}

static umedit_int ll_min_child(struct ccmmheap* self, umedit_int i) {
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

static void ll_add_tail_elem(struct ccmmheap* self) {
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

void ccmmheap_add(struct ccmmheap* self, void* elem) {
  if (self->capacity == 0 || elem == 0) {
    ccloge("mmheap_add invalid argument");
    return;
  }
  if (self->size >= self->capacity) {
    self->a = ccrawrelloc(self->a, self->capacity, self->capacity*2);
    self->capacity *= 2;
  }
  self->a[self->size++] = (unsign_ptr)elem;
  ll_add_tail_elem(self);
}

void* ccmmheap_del(struct ccmmheap* self, umedit_int i) {
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

nauty_bool ccisprime(umedit_int n) {
  uright_int i = 0;
  if (n == 2) return true;
  if (n == 1 || (n % 2) == 0) return false;
  for (i = 3; i*i <= n; i += 2) {
    if ((n % i) == 0) return false;
  }
  return true;
}

/* return prime is less than 2^bits and greater then 2^(bits-1) */
umedit_int ccmidprime(nauty_byte bits) {
  umedit_int i = 0, n = 0, mid = 0, m = (1 << bits);
  umedit_int dn = 0, dm = 0; /* distance */
  umedit_int nprime = 0, mprime = 0;
  if (m == 0) return 0; /* max value of bits is 31 */
  if (bits < 3) return bits + 1; /* 1(2^0) 2(2^1) 4(2^2) => 1 2 3 */
  n = (1 << (bits - 1)); /* even number */
  mid = 3 * (n >> 1); /* middle value between n and m, even number */
  for (i = mid - 1; i > n; i -= 2) {
    if (ccisprime(i)) {
      dn = i - n;
      nprime = i;
      break;
    }
  }
  for (i = mid + 1; i < m; i += 2) {
    if (ccisprime(i)) {
      dm = m - i;
      mprime = i;
      break;
    }
  }
  return (dn > dm ? nprime : mprime);
}

void cchashtable_init(struct cchashtable* self, nauty_byte sizebits, int offsetofnext, umedit_int (*getkey)(void*)) {
  if (sizebits > 30) {
    ccloge("size too large");
    return;
  }
  if (sizebits < 3) {
    sizebits = 3;
  }
  self->slotbits = sizebits;
  self->nslot = ccmidprime(sizebits);
  self->slot = (struct cchashslot*)ccrawalloc_init(sizeof(struct cchashslot)*self->nslot);
  self->nbucket = 0;
  self->offsetofnext = (ushort_int)offsetofnext;
  self->getkey = getkey;
}

void cchashtable_free(struct cchashtable* self) {
  if (self->slot) {
    ccrawfree(self->slot);
    self->slot = 0;
  }
  cczeron(self, sizeof(struct cchashtable));
}

static umedit_int llgethashval(struct cchashtable* self, void* elem) {
  return (self->getkey(elem) % self->nslot);
}

static void llsetelemnext(struct cchashtable* self, void* elem, void* next) {
  *((void**)((nauty_byte*)elem + self->offsetofnext)) = next;
}

static void* llgetnextelem(struct cchashtable* self, void* elem) {
  return *((void**)((nauty_byte*)elem + self->offsetofnext));
}

void cchashtable_add(struct cchashtable* self, void* elem) {
  struct cchashslot* slot = 0;
  if (elem == 0) return;
  slot = self->slot + llgethashval(self, elem);
  llsetelemnext(self, elem, slot->next);
  slot->next = elem;
  self->nbucket += 1;
}

void cchashtable_foreach(struct cchashtable* self, void (*cb)(void*)) {
  struct cchashslot* slot = self->slot;
  struct cchashslot* end = slot + self->nslot;
  struct cchashslot* elem = 0;
  if (self->slot == 0) return;
  for (; slot < end; ++slot) {
    elem = slot->next;
    while (elem) {
      cb(elem);
      elem = llgetnextelem(self, elem);
    }
  }
}

void* cchashtable_find(struct cchashtable* self, umedit_int key) {
  struct cchashslot* slot = 0;
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

void* cchashtable_del(struct cchashtable* self, umedit_int key) {
  struct cchashslot* slot = 0;
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

void ccbackhash_init(struct ccbackhash* self, nauty_byte initsizebits, int offsetofnext, umedit_int (*getkey)(void*)) {
  cchashtable_init(&self->a, initsizebits, offsetofnext, getkey);
  self->cur = &(self->a);
  self->b.slot = 0;
  self->old = &(self->b);
}

void ccbackhash_free(struct ccbackhash* self) {
  cchashtable_free(&self->a);
  cchashtable_free(&self->b);
  self->cur = self->old = 0;
}

static nauty_bool ll_need_to_enlarge(struct ccbackhash* self) {
  struct cchashtable* t = self->cur;
  return (t->nbucket > t->nslot * 2);
}

void ccbackhash_add(struct ccbackhash* self, void* elem) {
  if (elem == 0) return;
  if (!ll_need_to_enlarge(self)) {
    cchashtable_add(self->cur, elem);
  } else {
    struct cchashtable oldtable = *(self->old);
    struct cchashtable* temp = 0;
    cchashtable_init(self->old, self->cur->slotbits*2, self->cur->offsetofnext, self->cur->getkey);
    /* switch curtable to new enlarged table */
    temp = self->cur;
    self->cur = self->old;
    self->old = temp;
    /* re-hash previous old table elements to new table */
    if (oldtable.slot && oldtable.nbucket) {
      struct cchashslot* slot = oldtable.slot;
      struct cchashslot* end = slot + oldtable.nslot;
      struct cchashslot* head = 0;
      for (; slot < end; ++slot) {
        head = slot;
        while (head->next) {
          void* elem = head->next;
          head->next = llgetnextelem(&oldtable, elem);
          cchashtable_add(self->cur, elem);
        }
      }
    }
    if (oldtable.slot) {
      cchashtable_free(&oldtable);
    }
    cchashtable_add(self->cur, elem);
  }
}

void* ccbackhash_find(struct ccbackhash* self, umedit_int key) {
  void* elem = cchashtable_find(self->cur, key);
  if (elem == 0 && self->old->slot) {
    return cchashtable_find(self->old, key);
  }
  return elem;
}

void* ccbackhash_del(struct ccbackhash* self, umedit_int key) {
  void* elem = cchashtable_del(self->cur, key);
  if (elem == 0 && self->old->slot) {
    return cchashtable_del(self->old, key);
  }
  return elem;
}



l_from l_from_cstring(const void* s) {
  l_from a = {0};
  if (s) {
    a.start = l_str(s);
    a.end = l_str(s) + strlen(l_cast(char*, s));
  }
  return a;
}

l_from l_from_lstring(const void* s, int len) {
  l_from a = {0};
  if (s && len > 0) {
    a.start = l_str(s);
    a.end = l_str(s) + len;
  }
  return a;
}




/** Core test **/

void ccthattest() {
  char buffer[] = "012345678";
  char* a = buffer;
  struct ccfrom from;
  struct ccheap heap = {0};
  struct ccheap* pheap = &heap;
  uoctet_int ainit[4] = {0};
#if defined(CCDEBUG)
  cclogd("CCDEBUG true");
#else
  cclogd("CCDEBUG false");
#endif
  ccassert(heap.start == 0);
  ccassert(heap.beyond == 0);
  pheap->start = (nauty_byte*)buffer;
  ccassert(*pheap->start == '0');
  ccassert(*pheap->start == *(pheap->start));
  ccassert(&pheap->start == &(pheap->start));
  ccassert(ainit[0] == 0);
  ccassert(ainit[1] == 0);
  ccassert(ainit[2] == 0);
  ccassert(ainit[3] == 0);
  /* basic types */
  ccassert(sizeof(uoctet_int) == 1);
  ccassert(sizeof(soctet_int) == 1);
  ccassert(sizeof(sshort_int) == 2);
  ccassert(sizeof(ushort_int) == 2);
  ccassert(sizeof(smedit_int) == 4);
  ccassert(sizeof(umedit_int) == 4);
  ccassert(sizeof(sright_int) == 8);
  ccassert(sizeof(uright_int) == 8);
  ccassert(sizeof(size_t) >= sizeof(void*));
  ccassert(sizeof(uright_int) >= sizeof(size_t));
  ccassert(sizeof(unsign_ptr) == sizeof(void*));
  ccassert(sizeof(signed_ptr) == sizeof(void*));
  ccassert(sizeof(int) >= 4);
  ccassert(sizeof(char) == 1);
  ccassert(sizeof(float) == 4);
  ccassert(sizeof(double) == 8);
  ccassert(sizeof(speed_real) == 4 || sizeof(speed_real) == 8);
  ccassert(sizeof(short_real) == 4);
  ccassert(sizeof(right_real) == 8);
  ccassert(sizeof(union cceight) == 8);
  ccassert(sizeof(struct ccstring) == CCSTRING_SIZEOF);
  ccassert(sizeof(struct ccmutex) >= CC_MUTEX_BYTES);
  ccassert(sizeof(struct ccrwlock) >= CC_RWLOCK_BYTES);
  ccassert(sizeof(struct cccondv) >= CC_CONDV_BYTES);
  ccassert(sizeof(struct ccthrkey) >= CC_THKEY_BYTES);
  ccassert(sizeof(struct ccthrid) >= CC_THRID_BYTES);
  /* value ranges */
  ccassert(CCM_UBYT_MAX == 255);
  ccassert(CCM_SBYT_MAX == 127);
  ccassert(CCM_SBYT_MIN == -127-1);
  ccassert((uoctet_int)CCM_SBYT_MIN == 0x80);
  ccassert((uoctet_int)CCM_SBYT_MIN == 128);
  ccassert(CCM_USHT_MAX == 65535);
  ccassert(CCM_SSHT_MAX == 32767);
  ccassert(CCM_SSHT_MIN == -32767-1);
  ccassert((ushort_int)CCM_SSHT_MIN == 32768);
  ccassert((ushort_int)CCM_SSHT_MIN == 0x8000);
  ccassert(CCM_UMED_MAX == 4294967295);
  ccassert(CCM_SMED_MAX == 2147483647);
  ccassert(CCM_SMED_MIN == -2147483647-1);
  ccassert((umedit_int)CCM_SMED_MIN == 2147483648);
  ccassert((umedit_int)CCM_SMED_MIN == 0x80000000);
  ccassert(CCM_UINT_MAX == 18446744073709551615ull);
  ccassert(CCM_SINT_MAX == 9223372036854775807ull);
  ccassert(CCM_SINT_MIN == -9223372036854775807-1);
  ccassert((uright_int)CCM_SINT_MIN == 9223372036854775808ull);
  ccassert((uright_int)CCM_SINT_MIN == 0x8000000000000000ull);
  /* memory operation */
  ccassert(ccmultof(0, 0) == 0);
  ccassert(ccmultof(0, 4) == 4);
  ccassert(ccmultof(4, 0) == 4);
  ccassert(ccmultof(4, 1) == 4);
  ccassert(ccmultof(4, 2) == 4);
  ccassert(ccmultof(4, 3) == 4);
  ccassert(ccmultof(4, 4) == 4);
  ccassert(ccmultof(4, 5) == 8);
  ccassert(ccmultofpow2(0, 0) == 1);
  ccassert(ccmultofpow2(0, 4) == 4);
  ccassert(ccmultofpow2(2, 0) == 4);
  ccassert(ccmultofpow2(2, 1) == 4);
  ccassert(ccmultofpow2(2, 2) == 4);
  ccassert(ccmultofpow2(2, 3) == 4);
  ccassert(ccmultofpow2(2, 4) == 4);
  ccassert(ccmultofpow2(2, 5) == 8);
  cccopyfromp(ccsetfrom(&from, a, 1), a+1);
  ccassert(*(a+1) == '0'); *(a+1) = '1';
  cccopyfromp(ccsetfrom(&from, a+3, 1), a+2);
  ccassert(*(a+2) == '3'); *(a+2) = '2';
  cccopyfromp(ccsetfrom(&from, a, 4), a+2);
  ccassert(*(a+2) == '0');
  ccassert(*(a+3) == '1');
  ccassert(*(a+4) == '2');
  ccassert(*(a+5) == '3');
  /* other tests */
  ccassert(ccisprime(0) == false);
  ccassert(ccisprime(1) == false);
  ccassert(ccisprime(2) == true);
  ccassert(ccisprime(3) == true);
  ccassert(ccisprime(4) == false);
  ccassert(ccisprime(5) == true);
  ccassert(ccisprime(CCM_UMED_MAX) == false);
}
