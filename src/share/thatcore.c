#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#define CCLIB_THATCORE
#include "thatcore.h"
#include "luacapi.h"

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
    if (*p == '%' || *p == 's') continue;
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
  size_t n;
  if (bytes <= 0) bytes = 1;
  n = (size_t)(bytes = (sright_int)ccmultofpow2(3, bytes));
  if (bytes <= 0 || (uright_int)bytes != (uright_int)n) {
    n = 0;
  }
  return n;
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

void* ccrawrelloc(void* buffer, sright_int size) {
  size_t bytes;

  if ((bytes = llgetallocsize(size)) == 0) {
    ccloge("ccrawrelloc too large %s", ccitos(size));
    return 0;
  }

  if (buffer == 0) {
    /* returned buffer is not initialized */
    return llrawalloc(bytes);
  }

  /* if the new size is larger, the value of the newly allocated portion is indeterminate */
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
  CC_DEBUG_ZONE(
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

struct ccstring* ccdefaultstr() {
  return &ccthread_getself()->defstr;
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
  return ccstrgetcstr(ccstrsetu(ccdefaultstr(), a));
}

const char* ccitos(sright_int a) {
  return ccstrgetcstr(ccstrseti(ccdefaultstr(), a));
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

void cclinknode_init(struct cclinknode* node) {
  node->next = node->prev = node;
}

nauty_bool cclinknode_isempty(struct cclinknode* node) {
  if (node->next == node) {
    ccassert(node->prev == node);
    return true;
  }
  return false;
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

/** IO notification facility **/

struct ccionfslot {
  struct ccsmplnode head;
};

struct ccionfnode {
  struct ccionfnode* bucket_next_dont_use_;
  struct ccionfnode* qnext; /* union with size/type/flag in ccmsghead */
};

struct ccpqueue {
  struct ccionfnode head;
  struct ccionfnode* tail;
};

struct ccionfpool {
  umedit_int nslot; /* prime number size not near 2^n */
  umedit_int nfreed, nbucket, qsize;
  struct ccpqueue queue; /* chain all ccionfmsg fifo */
  struct ccsmplnode freelist;
  struct ccionfslot slot[1];
};

struct ccionfmgr {
  struct ccionfpool* pool;
};

struct ccionfevt;
struct ccionfmsg;

nauty_bool ccionfevt_isempty(struct ccionfevt* self);
void ccionfevt_setempty(struct ccionfevt* self);
struct ccionfmsg* ccionfmsg_new(struct ccionfevt* event);
struct ccionfmsg* ccionfmsg_set(struct ccionfmsg* self, struct ccionfevt* event);

void ccpqueue_init(struct ccpqueue* self) {
  self->head.qnext = self->tail = &self->head;
}

void ccpqueue_push(struct ccpqueue* self, struct ccionfnode* newnode) {
  newnode->qnext = self->tail->qnext;
  self->tail->qnext = newnode;
  self->tail = newnode;
}

nauty_bool ccpqueue_isempty(struct ccpqueue* self) {
  return (self->head.qnext == &self->head);
}

struct ccionfnode* ccpqueue_pop(struct ccpqueue* self) {
  struct ccionfnode* node = 0;
  if (ccpqueue_isempty(self)) {
    return 0;
  }
  node = self->head.qnext;
  self->head.qnext = node->qnext;
  if (self->tail == node) {
    self->tail = &self->head;
  }
  return node;
}

/** Thread and synchronization **/

/**
 * global variable
 */

struct ccglobal {
  struct ccthread* master;
  struct ccthrkey thrkey;
  struct ccdqueue thrdq;
  struct ccdqueue workq;
  struct ccdqueue freewq;
  struct ccsqueue msgrxq;
  struct ccsqueue freemq;
};

static struct ccglobal* ccG;

static nauty_bool llsetthrkeydata(struct ccthread* thrd) {
  return ccthrkey_setdata(&ccG->thrkey, thrd);
}

static void llglobal_init(struct ccglobal* self, struct ccthread* master) {
  cczeron(self, sizeof(struct ccglobal));
  ccdqueue_init(&self->thrdq);
  ccdqueue_init(&self->workq);
  ccdqueue_init(&self->freewq);
  ccsqueue_init(&self->msgrxq);
  ccsqueue_init(&self->freemq);
  ccG = self;
  self->master = master;
  ccthrkey_init(&self->thrkey);
  llsetthrkeydata(master);
}

static void llglobal_free(struct ccglobal* self) {
  ccthrkey_free(&self->thrkey);
}

static struct ccsqueue* llglobalmq() {
  return &(ccG->msgrxq);
}

/**
 * thread
 */

static void llthread_lock(struct ccthread* self) {
  ccmutex_lock(&self->mutex);
}

static void llthread_unlock(struct ccthread* self) {
  ccmutex_unlock(&self->mutex);
}

static struct ccthread* llgetidlethread() {
  struct ccthread* thrd;
  thrd = (struct ccthread*)ccdqueue_pop(&ccG->thrdq);
  /* adjust the spriq */
  return thrd;
}

static void llparsecmdline(int argc, char** argv) {
  (void)argc;
  (void)argv;
}

int llmainthreadfunc(int (*start)(), int argc, char** argv) {
  int n = 0;
  struct ccglobal g;
  struct ccthread master;
  ccthread_init(&master);
  master.id = ccplat_selfthread();
  llglobal_init(&g, &master);
  llparsecmdline(argc, argv);
  n = start();
  llglobal_free(&g);
  ccthread_free(&master);
  return n;
}

static void* llthreadfunc(void* para) {
  struct ccthread* self = (struct ccthread*)para;
  int n = 0;
  llsetthrkeydata(self);
  n = self->start();
  ccthread_free(self);
  return (void*)(signed_ptr)n;
}

struct ccthread* ccthread_getself() {
  return (struct ccthread*)ccthrkey_getdata(&ccG->thrkey);
}

struct ccthread* ccthread_getmaster() {
  return ccG->master;
}

void ccthread_init(struct ccthread* self) {
  cczeron(self, sizeof(struct ccthread));
  cclinknode_init(&self->node);
  ccdqueue_init(&self->workrxq);
  ccdqueue_init(&self->workq);
  ccmutex_init(&self->mutex);
  cccondv_init(&self->condv);
  ccsqueue_init(&self->msgq);
  ccsqueue_init(&self->freeco);
  self->defstr = ccemptystr();
}

void ccthread_free(struct ccthread* self) {
  self->start = 0;
  if (self->L) {
   cclua_close(self->L);
   self->L = 0;
  }
  ccstrfree(&self->defstr);
}

nauty_bool ccthread_start(struct ccthread* self, int (*start)()) {
  if (self->start) {
    ccloge("thread already started");
    return false;
  }
  self->start = start;
  self->L = cclua_newstate();
  if (!ccplat_createthread(&self->id, llthreadfunc, self)) {
    cczeron(&self->id, sizeof(struct ccthrid));
    return false;
  }
  return true;
}

void ccthread_sleep(uright_int us) {
  ccplat_threadsleep(us);
}

void ccthread_exit() {
  ccthread_free(ccthread_getself());
  ccplat_threadexit();
}

int ccthread_join(struct ccthread* self) {
  return ccplat_threadjoin(&self->id);
}

/**
 * message
 */

static nauty_bool llislocalmsg(struct ccmsghead* msg) {
  return (msg->svid & CCMSGTYPE_REMOTE) == 0;
}

static nauty_bool llismastermsg(struct ccmsghead* msg) {
  return (msg->svid & CCMSGTYPE_MASTER);
}

static void llsendremotemsg(struct ccmsghead* msg) {
  (void)msg;
}

void ccservice_sendmsg(struct ccmsghead* msg) {
  if (llislocalmsg(msg)) {
    struct ccthread* master = ccthread_getmaster();
    llthread_lock(master);
    /* append msg to global mq */
    ccsqueue_push(llglobalmq(), &msg->node);
    /* if master is sleep, wakeup it */
    if ((master->runstatus & CCRUNSTATUS_SLEEP) &&
        (master->runstatus & CCRUNSTATUS_WAKEUP_TRIGGERED) == 0) {
      master->runstatus |= CCRUNSTATUS_WAKEUP_TRIGGERED;
      /* TODO: wakeup master */
    }
    llthread_unlock(master);
  } else {
    llsendremotemsg(msg);
  }
}

/**
 * service
 */

static struct ccservice* lldestservice(struct ccmsghead* msg) { /* called by master only */
  (void)msg;
  return 0; /* TODO: (struct ccservice*)msg->svid;  find server by svid */
}

static nauty_bool llislservice(struct ccservice* self) {
  return (self->iflag & CCSVTYPE_LSERVICE);
}

static nauty_bool lliscservice(struct ccservice* self) {
  return (self->iflag & CCSVTYPE_CSERVICE);
}

void llmaster_dispatch(struct ccthread* master) {
  struct ccsqueue queue;
  struct ccsqueue freeq;
  struct ccmsghead* msg;
  struct ccservice* sv;

  /* get all global messages */
  llthread_lock(master);
  queue = *llglobalmq();
  ccsqueue_init(llglobalmq());
  llthread_unlock(master);

  /* prepare master's messages */
  while ((msg = (struct ccmsghead*)ccsqueue_pop(&queue))) {
    if (llismastermsg(msg)) {
      ccsqueue_push(&master->msgq, &msg->node);
    }
  }

  /* handle master's messages */
  ccsqueue_init(&freeq);
  while ((msg = (struct ccmsghead*)ccsqueue_pop(&master->msgq))) {
    /* TODO: how to handle master's messages */
    ccsqueue_push(&freeq, &msg->node);
  }

  /* dispatch service's messages */
  while ((msg = (struct ccmsghead*)ccsqueue_pop(&queue))) {
    nauty_bool newattach = false;

    /* get message's dest service */
    sv = lldestservice(msg);
    if (sv == 0) {
      ccsqueue_push(&freeq, &msg->node);
      continue;
    }

    if (sv->thrd) {
      nauty_bool done = false;
      llthread_lock(sv->thrd);
      done = (sv->oflag & CCSVFLAG_DONE) != 0;
      llthread_unlock(sv->thrd);
      if (done) {
        sv->thrd = 0;
        ccsqueue_push(&freeq, &msg->node);
        /* TODO: reset and remove servie to free list */
        continue;
      }
    } else {
      if (sv->oflag & CCSVFLAG_DONE) {
        ccsqueue_push(&freeq, &msg->node);
        /* TODO: reset and remove servie to free list */
        continue;
      }
      /* attach a thread to handle */
      sv->thrd = llgetidlethread();
      if (sv->thrd == 0) {
        /* TODO: thread busy, need pending */
      }
      newattach = true;
    }

    llthread_lock(sv->thrd);
    msg->extra = sv;
    ccsqueue_push(&sv->rxmq, &msg->node);
    if (newattach) {
      ccdqueue_push(&sv->thrd->workrxq, &sv->node);
    }
    llthread_unlock(sv->thrd);
  }

  if (!ccsqueue_isempty(&freeq)) {
    llthread_lock(master);
    /* TODO: add free msg to global free queue */
    llthread_unlock(master);
  }
}

void llthread_runservice(struct ccthread* thrd) {
  struct ccdqueue doneq;
  struct ccsqueue freemq;
  struct ccservice* sv;
  struct cclinknode* head;
  struct cclinknode* node;
  struct ccmsghead* msg;
  int n = 0;

  llthread_lock(thrd);
  ccdqueue_pushqueue(&thrd->workq, &thrd->workrxq);
  llthread_unlock(thrd);

  if (ccdqueue_isempty(&thrd->workq)) {
    /* no services need to run */
    return;
  }

  head = &(thrd->workq.head);
  node = head->next;
  for (; node != head; node = node->next) {
    sv = (struct ccservice*)node;
    if (!ccsqueue_isempty(&sv->rxmq)) {
      llthread_lock(thrd);
      ccsqueue_pushqueue(&thrd->msgq, &sv->rxmq);
      llthread_unlock(thrd);
    }
  }

  if (ccsqueue_isempty(&thrd->msgq)) {
    /* current services have no messages */
    return;
  }

  ccdqueue_init(&doneq);
  ccsqueue_init(&freemq);
  while ((msg = (struct ccmsghead*)ccsqueue_pop(&thrd->msgq))) {
    sv = (struct ccservice*)msg->extra;
    if (sv->iflag & CCSVFLAG_DONE) {
      ccsqueue_push(&freemq, &msg->node);
      continue;
    }
    if (llislservice(sv)) {
      /* TODO: lua service */
    } else if (lliscservice(sv)) {
      if (sv->co == 0) {
        if ((sv->co = (struct ccluaco*)ccsqueue_pop(&thrd->freeco)) == 0) {
          /* TODO: how to allocate ccluaco */
          /* sv->co = ccluaco_create(thrd, sv->u.cofunc, sv->udata); */
        }
      }
      if (sv->co->owner != thrd) {
        ccloge("runservice invalide luaco");
      }
      sv->co->msg = msg;
      n = ccluaco_resume(sv->co);
      sv->co->msg = 0;
    } else {
      n = sv->u.func(sv->udata, &msg->node);
    }
    /* service run completed */
    if (n == 0) {
      sv->iflag |= CCSVFLAG_DONE;
      ccdqueue_push(&doneq, cclinknode_remove(&sv->node));
      if (sv->co) {
        ccsqueue_push(&thrd->freeco, &sv->co->node);
        sv->co = 0;
      }
    }
    /* free handled message */
    ccsqueue_push(&freemq, &msg->node);
  }

  if (!ccdqueue_isempty(&doneq)) {
    llthread_lock(thrd);
    while ((sv = (struct ccservice*)ccdqueue_pop(&doneq))) {
      sv->iflag &= (~CCSVFLAG_DONE);
      sv->oflag |= CCSVFLAG_DONE;
    }
    llthread_unlock(thrd);
  }

  if (!ccsqueue_isempty(&freemq)) {
    struct ccthread* master = ccthread_getmaster();
    llthread_lock(master);
    /* TODO: add free msg to global free queue */
    llthread_unlock(master);
  }
}

/** Other functions **/

nauty_bool ccisprime(umedit_int n) {
  sright_int i = 0;
  if (n == 2) return true;
  if (n == 1 || (n % 2) == 0) return false; 
  for (i = 3; i*i <= n; i += 2) {
    if ((n % i) == 0) return false;
  }
  return true;
}

umedit_int cchashprime(uoctet_int bits) {
  umedit_int i = 0, n = 0, mid = 0, m = (1 << bits);
  umedit_int dn = 0, dm = 0; /* distance */
  umedit_int nprime = 0, mprime = 0;
  if (m == 0) return 0; /* max value of bits is 31 */
  if (bits < 3) return bits + 1; /* 1(2^0) 2(2^1) 4(2^2) => 1 2 3 */
  n = (1 << (bits - 1)); /* even number */
  mid = 3 * n / 2; /* middle value between n and m, even number */
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

/** Core test **/

void ccthattest() {
  char buffer[] = "012345678";
  char* a = buffer;
  struct ccfrom from;
  struct ccheap heap = {0};
  uoctet_int ainit[4] = {0};
#if defined(CCDEBUG)
  cclogd("CCDEBUG true");
#else
  cclogd("CCDEBUG false");
#endif
  ccassert(heap.start == 0);
  ccassert(heap.beyond == 0);
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
  ccassert(sizeof(struct ccthread) >= CC_THRID_BYTES);
  /* value ranges */
  ccassert(CC_UBYT_MAX == 255);
  ccassert(CC_SBYT_MAX == 127);
  ccassert(CC_SBYT_MIN == -127-1);
  ccassert((uoctet_int)CC_SBYT_MIN == 0x80);
  ccassert((uoctet_int)CC_SBYT_MIN == 128);
  ccassert(CC_USHT_MAX == 65535);
  ccassert(CC_SSHT_MAX == 32767);
  ccassert(CC_SSHT_MIN == -32767-1);
  ccassert((ushort_int)CC_SSHT_MIN == 32768);
  ccassert((ushort_int)CC_SSHT_MIN == 0x8000);
  ccassert(CC_UMED_MAX == 4294967295);
  ccassert(CC_SMED_MAX == 2147483647);
  ccassert(CC_SMED_MIN == -2147483647-1);
  ccassert((umedit_int)CC_SMED_MIN == 2147483648);
  ccassert((umedit_int)CC_SMED_MIN == 0x80000000);
  ccassert(CC_UINT_MAX == 18446744073709551615ull);
  ccassert(CC_SINT_MAX == 9223372036854775807ull);
  ccassert(CC_SINT_MIN == -9223372036854775807-1);
  ccassert((uright_int)CC_SINT_MIN == 9223372036854775808ull);
  ccassert((uright_int)CC_SINT_MIN == 0x8000000000000000ull);
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
  /* string */
  ccutos(CC_UINT_MAX);
  ccassert(ccstrgetlen(ccdefaultstr()) < CCSTRING_STATIC_CHARS);
  ccassert(ccstrcapacity(ccdefaultstr()) == CCSTRING_STATIC_CHARS);
  /* other tests */
  ccassert(ccisprime(0) == false);
  ccassert(ccisprime(1) == false);
  ccassert(ccisprime(2) == true);
  ccassert(ccisprime(3) == true);
  ccassert(ccisprime(4) == false);
  ccassert(ccisprime(5) == true);
  ccassert(ccisprime(CC_UMED_MAX) == false);
}
