/*
各任务可以生成新的子任务，新生成的任务挂接在空闲任务列表中。
主线程负责对任务进行分配，每个任务由一个唯一的id来表示，可以跨越网络进行通信。
当有对应任务的消息到来时，如果这个任务还没有分配线程处理，则根据负载为其分配一个线程；
任务分配到线程后，相当于变成线程数据的一部分，借助线程的mutex将消息插入到任务的消息队列中；

1. 任务拥有自己的消息队列，任务可以该任意其他任务发送消息，任务可以跨越网络存在
2. 一个任务在另一个任务发送消息时，先看目标任务是本地任务还是远端任务，远端任务需要先进行网络操作
3. 如果是本地任务，先看这个任务有没有分配线程（如果保证这个变量访问的一致性？只能由主线程去进行分配），
　　所以将消息插入主线程的rxq中，然后触发主线程分配这个消息，如何触发？哪些fd可以在epoll中使用？
4. 主线程给任务分配线程后，一条链接链入任务消息队列，一条链接链入线程rxq中，线程按顺序处理完消息后要将消息标记未已处理，主线程在插入任务消息队列或任务执行完时好将消息移到空闲消息队列
5. 线程如何知道哪些任务正在由自己处理？
*/

/** 事件数据传递过程 **
EPOLL事件数据{int fd; uint32_t events; void* ud}
事件数据加入事件等待队列中，该队列中的数据需要按照当前线程负载分配给不同的线程处理（或递交给已经处理该任务的线程处理），事件队列数据{node; int fd; uint32_t events; void* ud; _int timestamp;}
将事件分配给线程处理，需要将将事件数据作为消息发送到对应线程的消息队列中
 ** 事件队列的处理方式 **
事件数据的一条链链入哈希桶，另一条链按到达顺序串联
如果有事件时处理1/10的事件然后不等待看是否有数据到来，然后再处理1/10，知道没有事件后再无限等待
分配自己线程的消息，先将等待的所有消息清除出来插入当前处理队列，每一次只处理所有这些消息，处理过程中到来的消息需等下一次处理
 ** c compatibility with c++ **
C compiler does not use mangling which c++ uses. So if you want to call c
interfaces in the c++ program, you have to clearly declare these c interfaces
as "extern c".
#ifdef __cplusplus
extern "C" {
#endif
// all your c interfaces here
#ifdef __cplusplus
}
#endif
While building c++ program, don't try to compile and link code with different
compilers, the best way is to compile and link all code with the compiler
currently use.
 ** C coding styles **
keep simple and clean, dont use plural and underscore
define struct directly, dont use typedef to redefine the name
dont use "get" if the words in the name is two or more.
creation style - use return value to create the structure
access style - use the 1st parameter named 'self' to access or modify */

#define L_MKSTR(a) #a
#define L_X_MKSTR(a) L_MKSTR(a)
#define L_MKFLSTR __FILE__ " (" L_X_MKSTR(__LINE__) ") "

#define l_assert(e) l_assert_func((e), (#e), L_MKFLSTR)                           /* 0:assert */
#define l_log_e(fmt, ...) l_logger_func("1[E] " L_MKFLSTR, (fmt), ## __VA_ARGS__) /* 1:error */
#define l_log_w(fmt, ...) l_logger_func("2[W] " L_MKFLSTR, (fmt), ## __VA_ARGS__) /* 2:warning */
#define l_log_i(fmt, ...) l_logger_func("3[I] " L_MKFLSTR, (fmt), ## __VA_ARGS__) /* 3:important */
#define l_log_d(fmt, ...) l_logger_func("4[D] " L_MKFLSTR, (fmt), ## __VA_ARGS__) /* 4:debug */

l_extern void l_assert_func(int pass, const void* expr, const void* fileline);
l_extern int l_logger_func(const void* tag, const void* fmt, ...);
l_extern void l_set_log_level(int level);
l_extern int l_get_log_level();


void l_assert_func(int pass, const void* expr, const void* fileline) {
  if (pass) {
    l_logger_func("4[D] ", "%sassert pass: %s", fileline, expr);
  } else {
    l_logger_func("0[E] ", "%sassert fail: %s", fileline, expr);
  }
}

static l_integer l_log_level = 2;
int l_logger_func(const void* tag, const void* fmt, ...) {
  int n = 0, level = tag[0] - '0';
  const l_byte* p = l_cast(const l_byte*, fmt);
  int printed = 0;
  if (level > l_log_level) return 0;
  for (; *p; ++p) {
    if (*p != '%') continue;
    if (*(p+1) == 0) {
      l_loge("l_logger_func invalid (1) %s", fmt);
      return 0;
    }
    p += 1;
    if (*p == '%' || *p == 's' || *p == 'p') continue;
    l_loge("l_logger_func invalid (2) %s", fmt);
    return 0;
  }
  if ((n = (int)fprintf(stdout, "%s", tag + 1)) < 0) {
    n = 0;
  }
  if (fmt) {
    va_list args;
    va_start(args, fmt);
    if ((printed = vfprintf(stdout, (const char*)fmt, args)) > 0) {
      n += (int)printed;
    }
    va_end(args);
  }
  if ((printed = fprintf(stdout, CCNEWLINE)) > 0) {
    n += (int)printed;
  }
  return n;
}

void l_setloglevel(int level) {
  l_loglevel = level;
}

int l_getloglevel() {
  return l_loglevel;
}




#define ccassert(e) ccimplassert((e), #e, __FILE__, __LINE__)          /* 0:assert */
#define ccloge(fmt, ...) ccimploutput(0, 1, "[E] ", (fmt), ## __VA_ARGS__) /* 1:error */
#define cclogw(fmt, ...) ccimploutput(0, 2, "[W] ", (fmt), ## __VA_ARGS__) /* 2:warning */
#define cclogi(fmt, ...) ccimploutput(0, 3, "[I] ", (fmt), ## __VA_ARGS__) /* 3:important */
#define cclogd(fmt, ...) ccimploutput(0, 4, "[D] ", (fmt), ## __VA_ARGS__) /* 4:debug - disabled in release version */
#define ccfprint(file, fmt, ...) ccimploutput(file, -1, 0, (fmt), ## __VA_ARGS__)
#define ccfprintln(file, fmt, ...) ccimploutput(file, -1, "L", (fmt), ## __VA_ARGS__)

CORE_API void ccimplassert(bool pass, const void* expr, const void* file, uint line);
CORE_API sint ccimploutput(struct ccfile* file, _int level, const void* tag, const void* fmt, ...);

void ccimplassertfunc(bool pass, const void* expr, const void* file, uint line) {
  if (pass) cclogd("assert pass: %s", expr);
  else ccimploutput(0, 0, "[E] ", "assert fail: %s at %s line %s", expr, file, ccutos(line));
}

static _int ccloglevel = 3;
_int ccimploutput(struct ccfile* file, _int level, const void* tag, const void* fmt, ...) {
  const byte* p = (const byte*)fmt;
  FILE* fout = 0;
  size_t sz = 0;
  if (level > 0 && level > ccloglevel) return 0;
  for (; *p; ++p) { /* consider only support %s, %f (double) "%-+10.10f", and %p (pointer) "%- 10p" */
    if (*p != '%') continue;
    if (*(p+1) == 0) { ccloge("invalid format string: %s", fmt); return 0; }
    p += 1;
    if (*p == '%' || *p == 's' || *p == 'f' || *p == 'p') continue;
    /* -+10.10f */
    if (*p == '-' || *p == '+' || *p == ' ' || *p == '0') {
      if (*(p+1) == 0) { ccloge("invalid format string: %s", fmt); return 0; }
      p += 1;
      if (*p == 's' || *p == 'f' || *p == 'p') continue;
    }
    /* +10.10f */
    if (*p == '-' || *p == '+' || *p == ' ' || *p == '0') {
      if (*(p+1) == 0) { ccloge("invalid format string: %s", fmt); return 0; }
      p += 1;
      if (*p == 's' || *p == 'f' || *p == 'p') continue;
    }
    /* 10.10f */
    if (*p >= '0' && *p <= '9') {
      if (*(p+1) == 0) { ccloge("invalid format string: %s", fmt); return 0; }
      p += 1;
      if (*p == 's' || *p == 'f' || *p == 'p') continue;
    }
    /* 8.10f */
    if (*p >= '0' && *p <= '9') {
      if (*(p+1) == 0) { ccloge("invalid format string: %s", fmt); return 0; }
      p += 1;
      if (*p == 's' || *p == 'f' || *p == 'p') continue;
    }
    /* .10f */
    if (*p == '.') {
      if (*(p+1) == 0 || *(p+1) < '0' || *(p+1) > '9' || *(p+2) == 0) {
        ccloge("invalid format string: %s", fmt); return 0;
      }
      p += 2;
    }
    /* 8f */
    if (*p >= '0' && *p <= '9') {
      if (*(p+1) == 0) { ccloge("invalid format string: %s", fmt); return 0; }
      p += 1;
      if (*p == 'f') continue;
    }
    /* f */
    if (*p == 'f') continue;
    ccloge("invalid format string: %s", fmt);
    return 0;
  }
  fout = (level >= 0) ? stdout : (file && file->impl_) ? (FILE*)file->impl_ : stdout;
  if (level >= 0 && tag) {
    sz += fwrite(tag, 1, strlen((const char*)tag), fout);
  }
  if (fmt) {
    va_list args;
    int res = 0;
    va_start(args, fmt);
    res = vfprintf(fout, (const char*)fmt, args);
    if (res > 0) sz += (size_t)res;
    va_end(args);
  }
  if (level >= 0 || (tag && ((const byte*)tag)[0] == 'L')) {
    sz += fwrite(CCNEWLINE, 1, CCNLSIZE, fout);
  }
  return (uint)sz;
}






l_from* l_setfrom(l_from* self, const void* start, l_integer bytes) {
  *self = l_fromn(start, bytes);
  return self;
}

l_dest l_dest(void* start, l_integer bytes) {
  l_dest dest = {0, 0};
  if (start && bytes > 0) {
    size_t n = (size_t)bytes;
    if ((l_uinteger)n != (l_uinteger)bytes) {
      l_loge("l_dest size too large %s", l_itos(bytes));
      return dest;
    }
    dest.start = (l_byte*)start;
    dest.beyond = dest.start + n;
  }
  return dest;
}

l_dest l_destrange(void* start, void* beyond) {
  l_dest dest = {0, 0};
  if (start < beyond) {
    dest.start = (l_byte*)start;
    dest.beyond = (l_byte*)beyond;
  }
  return dest;
}

const l_dest* l_destheap(const l_heap* heap) {
  return (const l_dest*)heap;
}


l_heap l_heap_allocrawbuffer(l_integer size) {
  l_heap heap = {0, 0};
  size_t bytes;
  if ((bytes = llgetallocsize(size)) == 0) {
    l_loge("allocrawbuffer too large %s", l_itos(size));
    return heap;
  }
  /* returned buffer is not initialized */
  if ((heap.start = (l_byte*)llrawalloc(bytes))) {
    heap.beyond = heap.start + bytes;
  }
  return heap;
}

l_heap l_heap_alloc(l_integer size) {
  l_heap heap = l_heap_allocrawbuffer(size);
  l_zero(heap.start, heap.beyond);
  return heap;
}

l_heap l_heap_allocfrom(l_integer size, l_from from) {
  l_heap heap = l_heap_allocrawbuffer(size);
  l_byte* beyond = heap.start + l_copyfromptodest(&from, l_destheap(&heap));
  if (beyond < heap.beyond) {
    l_zero(beyond, heap.beyond);
  }
  return heap;
}

void l_heap_relloc(l_heap* self, l_integer newsize) {
  size_t n;
  void* buffer;

  if (self->start == 0 || (n = llgetallocsize(newsize)) == 0) {
    l_loge("l_heap_relloc too large %s", l_itos(newsize));
    l_heap_free(self);
    return;
  }

  /* already large enough */
  if (self->start + n <= self->beyond) {
    self->beyond = self->start + n;
    return;
  }

  /* request size is larger */
  if ((buffer = llrawrelloc(self->start, n)) == 0) {
    l_loge("l_heap_relloc failed %s", l_utos(n));
    return;
  }
  l_zero(self->beyond, buffer + n);
  self->start = (l_byte*)buffer;
  self->beyond = self->start + n;
}

void l_heap_free(l_heap* self) {
  if (self->start == 0) return;
  free(self->start);
  self->start = 0;
  self->beyond = 0;
}



static l_integer llcopy(const l_byte* start, size_t n, l_byte* dest) {
  if (dest + n <= start || dest >= start + n) {
    memcpy(dest, start, n);
  } else {
    memmove(dest, start, n);
  }
  if ((l_uinteger)((l_integer)n) != (l_uinteger)n) {
    l_loge("llcopy size too large %s", l_utos(n));
  }
  return (l_integer)n;
}

l_integer l_copyfromptodest(const l_from* from, const l_dest* dest) {
  size_t nfrom, ndest;
  if (from->start >= from->beyond) return 0;
  if (dest->start >= dest->beyond) return 0;
  nfrom = from->beyond - from->start;
  ndest = dest->beyond - dest->start;
  return llcopy(from->start, (ndest < nfrom ? ndest : nfrom), dest->start);
}

l_integer l_copyfrom(l_from from, void* dest) {
  return l_copyfromp(&from, dest);
}

l_integer l_copyfromp(const l_from* from, void* dest) {
  if (from->start < from->beyond && dest) {
    return llcopy(from->start, from->beyond - from->start, (l_byte*)dest);
  }
  return 0;
}

static l_integer llrcopy(const l_byte* start, size_t n, l_byte* dest) {
  if (dest + n <= start || dest >= start + n) {
    const l_byte* beyond = start + n;
    while (beyond-- > start) {
      *dest++ = *beyond;
    }
  } else {
    l_byte* last = dest + n - 1;
    memmove(dest, start, n); /* copy content first */
    while (dest < last) { /* and do reverse */
      l_byte ch = *dest;
      *dest++ = *last;
      *last-- = ch;
    }
  }
  if ((l_uinteger)((l_integer)n) != (l_uinteger)n) {
    l_loge("llrcopy size too large %s", l_utos(n));
  }
  return (l_integer)n;
}

l_integer l_rcopyfromp(const l_from* from, void* dest) {
  if (from->start < from->beyond && dest) {
    return llrcopy(from->start, from->beyond - from->start, (l_byte*)dest);
  }
  return 0;
}

l_integer l_rcopyfromptodest(const l_from* from, const l_dest* dest) {
  size_t nfrom, ndest;
  if (from->start >= from->beyond) return 0;
  if (dest->start >= dest->beyond) return 0;
  nfrom = from->beyond - from->start;
  ndest = dest->beyond - dest->start;
  return llrcopy(from->start, (ndest < nfrom ? ndest : nfrom), dest->start);
}

typedef struct {
  l_byte* start;
  l_byte* end;
} l_dest;

l_extern l_dest l_heap_alloc(sright_int size);
l_extern l_dest l_heap_allocrawbuffer(sright_int size);
l_extern l_dest l_heap_allocfrom(sright_int size, l_from from);
l_extern void l_heap_relloc(l_heap* self, sright_int newsize);
l_extern void l_heap_free(l_heap* self);

l_extern l_integer l_copy(const l_byte* fromstart, const l_byte* frombeyond, l_byte* dest);
l_extern l_integer l_copyn(const l_byte* from, sright_int n, l_byte* dest);
l_extern l_integer l_copyfrom(l_from from, void* dest);
l_extern l_integer l_copyfromp(const l_from* from, void* dest);
l_extern l_integer l_copyfromtodest(l_from from, const l_dest* dest);
l_extern l_integer l_copyfromptodest(const l_from* from, const l_dest* dest);

l_extern l_integer l_rcopy(const l_byte* fromstart, const l_byte* frombeyond, l_byte* dest);
l_extern l_integer l_rcopyn(const l_byte* from, l_int n, l_byte* dest);
l_extern l_integer l_rcopyfrom(l_from from, void* dest);
l_extern l_integer l_rcopyfromp(const l_from* from, void* dest);
l_extern l_integer l_rcopyfromtodest(l_from from, const l_dest* dest);
l_extern l_integer l_rcopyfromptodest(const l_from* from, const l_dest* dest);

l_extern l_from l_fromp(const void* start, const void* beyond);
l_extern l_from l_fromn(const void* start, l_integer bytes);
l_extern l_from l_fromcstr(const void* cstr);
l_extern l_dest l_dest(void* start, l_integer bytes);
l_extern l_dest l_destrange(void* start, void* beyond);

l_extern l_from* l_setfrom(l_from* self, const void* start, l_integer bytes);
l_extern l_from* l_setfromcstr(l_from* self, const void* cstr);
l_extern l_from* l_setfromrange(l_from* self, const void* start, const void* beyond);
l_extern l_dest* l_setdest(l_dest* self, void* start, l_integer bytes);
l_extern l_dest* l_setdestrange(l_dest* self, void* start, void* beyond);
l_extern const l_dest* l_destheap(const l_heap* heap);

/** String **/

#define CCSTRING_SIZEOF 32
#define CCSTRING_STATIC_CHARS 30

typedef struct l_string {
  l_heap heap;
  sright_int len;
  uoctet_int a[CCSTRING_SIZEOF-sizeof(l_heap)-sizeof(sright_int)-1];
  uoctet_int flag; /* flag==0xFF ? heap-string : stack-string */
} l_string; /* a sequence of bytes with zero terminated */

l_extern const char* l_utos(uright_int a);
l_extern const char* l_itos(sright_int a);
l_extern l_string l_emptystr();
l_extern l_string l_strfromu(uright_int a);
l_extern l_string l_strfromuf(uright_int a, sright_int fmt);
l_extern l_string l_strfromi(sright_int a);
l_extern l_string l_strfromif(sright_int a, sright_int fmt);

l_extern l_string l_string_emptystr();
l_extern sright_int l_string_getlen(const l_string* self);
l_extern const char* l_string_getcstr(const l_string* self);
l_extern int l_string_equalcstr(const l_string* self, const void* cstr);

l_extern void l_string_free(l_string* self);
l_extern void l_string_setempty(l_string* self);
l_extern void l_string_setcstr(l_string* self, const void* cstr);

l_extern l_from l_string_getfrom(const l_string* s);
l_extern int l_string_contain(l_from s, nauty_char ch);
l_extern int l_string_containp(const l_from* s, nauty_char ch);


/** String **/

#define LLSMASK 0x0700
#define LLNSIGN 0x0100
#define CCPSIGN 0x0200
#define CCESIGN 0x0400
#define CCZFILL 0x0800
#define CCJUSTL 0x1000
#define CCFORCE 0x2000
#define CCTOHEX 0x4000

static const l_byte l_hexchars[] = "0123456789abcdef";

static l_integer llutos(l_uinteger n, l_integer minlen, void* to) {
  /* 64-bit unsigned int max value 18446744073709552046
     it is 20 characters, can be contained in l_string staticly */
  l_byte a[CCSTRING_STATIC_CHARS+1] = {0};
  l_byte* dest = (l_byte*)to;
  l_uinteger flag = (minlen & 0x7f00);
  l_integer i = 0, end = 0;
  if (dest == 0) return 0;
  if (flag & CCTOHEX) {
    a[i++] = l_hexchars[n & 0x0f];
    while ((n >>= 4)) {
      a[i++] = l_hexchars[n & 0x0f];
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
    l_assert(i <= CCSTRING_STATIC_CHARS);
  )
  while(i > 0) *dest++ = a[--i];
  *dest = 0;
  return dest - (l_byte*)to;
}

int l_strequal(l_from a, l_from b) {
  if (a.beyond - a.start != b.beyond - b.start) return false;
  while (a.start < a.beyond) {
    if (*(a.start++) != *(b.start++)) return false;
  }
  return true;
}

l_integer l_strgetlen(const l_string* self) {
  return (self->flag == 0xFF ? self->len : self->flag);
}

int l_strisempty(l_string* self) {
  return l_strgetlen(self) == 0;
}

const char* l_strgetcstr(const l_string* self) {
  return (self->flag == 0xFF ? (const char*)self->heap.start : (const char*)self);
}

l_from l_strgetfrom(const l_string* s) {
  return l_fromn(l_strgetcstr(s), l_strgetlen(s));
}

l_from l_string_getfrom(const l_string* s) {
  return l_fromn(l_strgetcstr(s), l_strgetlen(s));
}

l_integer l_strcapacity(const l_string* self) {
  if (self->flag == 0xFF) {
    const l_heap* heap = &self->heap;
    return (heap->start < heap->beyond) ? (heap->beyond - heap->start - 1) : 0;
  }
  return CCSTRING_STATIC_CHARS;
}

static l_string* llstrsetlen(l_string* self, l_integer len) {
  (self->flag == 0xFF) ? (self->len = len) : (self->flag = (l_byte)len);
  return self;
}

static nauty_char* llstrenlarge(l_string* self, l_integer bytes) {
  if (l_strcapacity(self) < bytes) {
    l_integer len = 2 * l_strgetlen(self); /* 1-byte for zero terminal */
    bytes = len >= (bytes+1) ? len : (bytes+1);
    if (self->flag == 0xFF) {
      l_heap_relloc(&self->heap, bytes);
      return self->heap.start;
    }
    if ((self->heap = l_heap_allocfrom(bytes, l_strgetfrom(self))).start == 0) {
      return 0;
    }
    self->len = self->flag;
    self->flag = 0xFF;
    return self->heap.start;
  }
  return (l_byte*)l_strgetcstr(self);
}

l_string l_emptystr() {
  return (l_string){{0}, 0, {0}, 0};
}

l_string l_strfromu(l_uinteger a) {
  return l_strfromuf(a, 0);
}

l_string l_strfromuf(l_uinteger a, l_integer fmt) {
  l_string s = l_emptystr();
  return *llstrsetlen(&s, llutos(a, fmt, &s));
}

l_string l_strfromi(l_integer a) {
  return l_strfromif(a, 0);
}

l_string l_strfromif(l_integer a, l_integer fmt) {
  return l_strfromuf(a < 0 ? (-a) : a, a < 0 ? (LLNSIGN | fmt) : fmt);
}

l_string* l_strsetuf(l_string* self, l_uinteger a, l_integer fmt) {
  return llstrsetlen(self, llutos(a, fmt, llstrenlarge(self, CCSTRING_STATIC_CHARS)));
}

l_string* l_strsetu(l_string* self, l_uinteger a) {
  return l_strsetuf(self, a, 0);
}

l_string* l_strsetif(l_string* self, l_integer a, l_integer fmt) {
  return llstrsetlen(self, llutos(a < 0 ? (-a) : a, (a < 0 ? (fmt | LLNSIGN) : fmt), llstrenlarge(self, CCSTRING_STATIC_CHARS)));
}

l_string* l_strseti(l_string* self, l_integer a) {
  return l_strsetif(self, a, 0);
}

void l_strfree(l_string* self) {
  if (self->flag == 0xFF) {
    l_heap_free(&self->heap);
    self->len = 0;
  }
  self->flag = 0;
}

const char* l_utos(l_uinteger a) {
  return l_strgetcstr(l_strsetu(l_thread_getdefstr(), a));
}

const char* l_itos(l_integer a) {
  return l_strgetcstr(l_strseti(l_thread_getdefstr(), a));
}


void l_string_free(l_string* self) {
  if (self->flag == 0xFF) {
    l_heap_free(&self->heap);
    self->len = 0;
  }
  self->flag = 0;
}

const char* l_string_getcstr(const l_string* self) {
  return (self->flag == 0xFF ? (const char*)self->heap.start : (const char*)self);
}

l_integer l_string_getlen(const l_string* self) {
  return (self->flag == 0xFF ? self->len : self->flag);
}

int l_string_equalcstr(const l_string* self, const void* cstr) {
  return l_strequal(l_fromn(l_string_getcstr(self), l_string_getlen(self)), l_fromcstr(cstr));
}

l_string l_string_emptystr() {
  return (l_string){{0}, 0, {0}, 0};
}

void l_string_setempty(l_string* self) {
  l_string_free(self);
  *self = l_string_emptystr();
}

void l_string_setcstr(l_string* self, const void* cstr) {
  l_integer n = strlen((const char*)cstr);
  nauty_char* p = llstrenlarge(self, n);
  llstrsetlen(self, l_copyfrom(l_fromn(cstr, n), p));
}

int l_string_contain(l_from s, nauty_char ch) {
  return l_string_containp(&s, ch);
}

int l_string_containp(const l_from* s, nauty_char ch) {
  const l_byte* start = s->start;
  if (start >= s->beyond) return false;
  while (start < s->beyond) {
    if (*start++ == (l_byte)ch) {
      return true;
    }
  }
  return false;
}

#if 0
CCINLINE nauty_char l_lower(nauty_char ch) {
  return (ch >= 'A' && ch <= 'Z') ? (ch + 32) : ch;
}

CCINLINE void l_lowerp(nauty_char* ch) {
  *ch = l_lower(*ch);
}

CCINLINE nauty_char l_upper(nauty_char ch) {
  return (ch >= 'a' && ch <= 'z') ? (ch - 32) : ch;
}

CCINLINE void l_upperp(nauty_char* ch) {
  *ch = l_upper(*ch);
}

CCINLINE void l_string_lower(nauty_char* s, int len) {
  while (len > 0) {
    l_lowerp(s + (--len));
  }
}

CCINLINE void l_string_upper(nauty_char* s, int len) {
  while (len > 0) {
    l_upperp(s + (--len));
  }
}

CCINLINE int l_isblank(int ch) {
  if (ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f') return 1;
  return 0;
}
#endif

/* string */
CORE_API const byte* ccitos(_int a);
CORE_API const byte* ccsitos(const void* s, _int a);
CORE_API const byte* ccistos(_int a, const void* s);
CORE_API const byte* ccisitos(_int a, const void* s, _int b);
CORE_API const byte* ccisisitos(_int a, const void* s, _int b, const void* sb, _int c);
CORE_API const byte* ccutos(uint a);
CORE_API const byte* ccsutos(const void* s, uint a);
CORE_API const byte* ccustos(uint a, const void* s);
CORE_API const byte* ccusutos(uint a, const void* s, uint b);
CORE_API const byte* ccususutos(uint a, const void* s, uint b, const void* sb, uint c);
CORE_API const byte* ccftos(double a);
CORE_API const byte* ccwstrztos(const ushort* ws);
CORE_API const byte* ccstrzcstr(const struct ccstrz* self);
CORE_API const byte* ccdatetos(const struct ccdate* date);
CORE_API struct ccstrz* ccdefaultstrz();
CORE_API struct ccstrz* ccstrzreset(struct ccstrz* self, struct ccfrom s);
CORE_API struct ccstrz* ccstrzresets(struct ccstrz* self, const void* s);
CORE_API struct ccstrz* ccstrzreseti(struct ccstrz* self, _int a);
CORE_API struct ccstrz* ccstrzresetim(struct ccstrz* self, _int a, uint minlen);
CORE_API struct ccstrz* ccstrzresetis(struct ccstrz* self, _int a, const void* s);
CORE_API struct ccstrz* ccstrzresetsi(struct ccstrz* self, const void* s, _int a);
CORE_API struct ccstrz* ccstrzresetisi(struct ccstrz* self, _int a, const void* s, _int b);
CORE_API struct ccstrz* ccstrzresetisisi(struct ccstrz* self, _int a, const void* s, _int b, const void* sb, _int c);
CORE_API struct ccstrz* ccstrzresetu(struct ccstrz* self, uint a);
CORE_API struct ccstrz* ccstrzresetum(struct ccstrz* self, uint a, uint minlen);
CORE_API struct ccstrz* ccstrzresetus(struct ccstrz* self, uint a, const void* s);
CORE_API struct ccstrz* ccstrzresetsu(struct ccstrz* self, const void* s, uint a);
CORE_API struct ccstrz* ccstrzresetusu(struct ccstrz* self, uint a, const void* s, uint b);
CORE_API struct ccstrz* ccstrzresetususu(struct ccstrz* self, uint a, const void* s, uint b, const void* sb, uint c);
CORE_API struct ccstrz* ccstrzresetf(struct ccstrz* self, double a);
CORE_API struct ccstrz* ccstrzadd(struct ccstrz* self, struct ccfrom s);
CORE_API struct ccstrz* ccstrzaddbyte(struct ccstrz* self, byte a);
CORE_API struct ccstrz* ccstrzadds(struct ccstrz* self, const void* s);
CORE_API struct ccstrz* ccstrzaddi(struct ccstrz* self, _int a);
CORE_API struct ccstrz* ccstrzaddu(struct ccstrz* self, uint a);
CORE_API struct ccstrz* ccstrzaddim(struct ccstrz* self, _int a, uint minlen);
CORE_API struct ccstrz* ccstrzaddum(struct ccstrz* self, uint a, uint minlen);
CORE_API struct ccstrz* ccstrzaddf(struct ccstrz* self, double a);
CORE_API struct ccstrz ccemptystrz();
CORE_API struct ccstrz ccstrzfrom(struct ccfrom s);
CORE_API struct ccstrz ccstrzfroms(const void* s);
CORE_API struct ccstrz ccstrzfromi(_int a);
CORE_API struct ccstrz ccstrzfromim(_int a, uint minlen);
CORE_API struct ccstrz ccstrzfromsi(const void* s, _int a);
CORE_API struct ccstrz ccstrzfromis(_int a, const void* s);
CORE_API struct ccstrz ccstrzfromisi(_int a, const void* s, _int b);
CORE_API struct ccstrz ccstrzfromisisi(_int a, const void* s, _int b, const void* sb, _int c);
CORE_API struct ccstrz ccstrzfromu(uint a);
CORE_API struct ccstrz ccstrzfromum(uint a, uint minlen);
CORE_API struct ccstrz ccstrzfromsu(const void* s, uint a);
CORE_API struct ccstrz ccstrzfromus(uint a, const void* s);
CORE_API struct ccstrz ccstrzfromusu(uint a, const void* s, uint b);
CORE_API struct ccstrz ccstrzfromususu(uint a, const void* s, uint b, const void* sb, uint c);
CORE_API struct ccstrz ccstrzfromf(double a);
CORE_API struct ccfrom ccstrzget(const struct ccstrz* self);
CORE_API uint ccstrzlen(const struct ccstrz* self);
CORE_API uint ccstrzmax(const struct ccstrz* self);
CORE_API void ccstrzfree(struct ccstrz* self);
CORE_API bool ccisemptystrz(const struct ccstrz* self);
CORE_API bool ccstrzequal(struct ccfrom a, struct ccfrom b);

static struct ccstring* llsfmt(struct ccfrom* s, int minlen, struct ccstring* (*func)(struct ccstring*, struct ccfrom), struct ccstring* self) {
  byte buffer[CCSTRING_STATIC_CHARS+1] = {0};
  byte* a = buffer;
  unsigned int flag = (minlen & 0x7f00);
  int blanks = 0;
  minlen = (minlen & 0xff);
  if (minlen > CCSTRING_STATIC_CHARS) minlen = CCSTRING_STATIC_CHARS;
  if (s->start < s->beyond) {
    const byte* beg = s->start;
    byte* end = a + minlen;
    if (s->beyond - s->start >= minlen) return self;
    if (flag & CCJUSTL) {
      while (beg < s->beyond) *a++ = *beg++;
      while (beg < end) *a++ = ' ';
    } else {
      blanks = minlen - (s->beyond - s->start);
      beg = a + blanks;
      while (blanks--) *a++ = ' ';
      while (beg < end) *a++ = *(s->start);
    }
  } else {
    blanks = minlen;
    while (blanks--) *a++ = ' ';
  }
  s->start = a;
  s->beyond = a + minlen;
  return func(self, *s);
}

union lldouble {
  double d;
  uint u;
};

static int llrtos(double n, int minlen, struct ccstring* dstr) {
  byte a[CCSTRING_STATIC_CHARS+1] = {0};
  byte* dest = (byte*)to;
  int i = 0, printed = 0;
  bool negative = false;
  uint exponent = 0;
  uint fraction = 0;
  union lldouble d;
  if (dest == 0) return 0;
  d.d = n;
  negative = (d.u & 0x8000000000000000) != 0;
  exponent = (d.u & 0x7FF0000000000000) >> 52;
  fraction = (d.u & 0x000FFFFFFFFFFFFF);
  if (exponent == 0 && fraction == 0) {
    if (negative) a[i++] = '-';
    else if (flag & LLFMTFLAG_PSIGN) a[i++] = '+';
    else if (flag & LLFMTFLAG_ESIGN) a[i++] = ' ';
    a[i++] = '0'; a[i++] = '.'; a[i++] = '0';
  } else if (exponent == 0x00000000000007FF) {
    if (fraction == 0) {
      if (negative) a[i++] = '-';
      else if (flag & LLFMTFLAG_PSIGN) a[i++] = '+';
      else if (flag & LLFMTFLAG_ESIGN) a[i++] = ' ';
      a[i++] = 'I'; a[i++] = 'n'; a[i++] = 'f'; a[i++] = 'i';
      a[i++] = 'n'; a[i++] = 'i'; a[i++] = 't'; a[i++] = 'y';
    } else {
      a[i++] = 'N'; a[i++] = 'a'; a[i++] = 'N';
    }
  } else {
    if (negative) a[i++] = '-';
    else if (flag & LLFMTFLAG_PSIGN) a[i++] = '+';
    else if (flag & LLFMTFLAG_ESIGN) a[i++] = ' ';
    /* printed does not include the additional null char */
    if ((printed = snprintf((char*)(a+i), CCSTRING_STATIC_CHARS+1-i, "%#g", n)) < 0) {
      ccloge("ccrtos snprintf fail %s", strerror(errno));
      return 0;
    }
    if ((i += printed) > CCSTRING_STATIC_CHARS) {
      i = CCSTRING_STATIC_CHARS;
    }
  }
  a[i] = 0;
  return llsfmt(ccfrom(a, i), flag, minlen, to);
}

static uint llrtos(double dval, struct ccstring* dstr) {
  byte a[CCSTRING_STATIC_CHARS+1] = {0};
  byte* p = a;
  bool negative = false;
  uint exponent = 0;
  uint fraction = 0;
  uint len = 0;
  union llfloat d;
  d.f = dval;
  negative = (d.u & 0x8000000000000000) != 0;
  exponent = (d.u & 0x7FF0000000000000) >> 52;
  fraction = (d.u & 0x000FFFFFFFFFFFFF);
  if (exponent == 0 && fraction == 0) {
    if (negative) *p++ = '-';
    *p++ = '0'; *p++ = '.'; *p++ = '0';
  } else if (exponent == 0x00000000000007FF) {
    if (fraction == 0) {
      if (negative) *p++ = '-';
      *p++ = 'I'; *p++ = 'n'; *p++ = 'f'; *p++ = 'i'; *p++ = 'n'; *p++ = 'i'; *p++ = 't'; *p++ = 'y';
    } else {
      *p++ = 'N'; *p++ = 'a'; *p++ = 'N';
    }
  } else {
    /* on success, the total number of characters written is returned.
    this count does not include the additional null-character
    automatically appended at the end of the string. */
    int n = snprintf((char*)a, 32, "%#f", dval);
    if (n > 0 && n <= CCSTRING_STATIC_CHARS) {
      p += n;
    } else {
      *p++ = '0'; *p++ = '.'; *p++ = '0';
      ccloge("ccrtos convert fail");
    }
  }
  len = p - a;
  cccopy(ccfrom(a, len+1), to);
  return len;
}

const char* ccutos(uint a) {

}

const char* ccitos(_int a) {

}

const char* ccrtos(double a) {

}

struct ccstring ccemptystr = {{0}, 0, {0}, 0};

struct ccstring ccstrfromu(uint a) {
  return ccstrfromuf(a, 0);
}

struct ccstring ccstrfromuf(uint a, int fmt) {
  struct ccstring s = ccemptystr;
  return *llstrsetlen(&s, llutos(a, fmt, &s));
}

struct ccstring ccstrfromi(_int a) {
  return ccstrfromif(a, 0);
}

struct ccstring ccstrfromif(_int a, int fmt) {
  return ccstrfromuf(a < 0 ? (-a) : a, a < 0 ? (LLNSIGN | fmt) : fmt);
}

void ccstrfree(struct ccstring* self) {
  if (self->flag == 0xFF) {
    ccfree(&self->heap);
    self->len = 0;
  }
  self->flag = 0;
}

bool ccstrequal(struct ccfrom a, struct ccfrom b) {
  if (a.beyond - a.start != b.beyond - b.start) return false;
  while (a.start < a.beyond) {
    if (*(a.start++) != *(b.start++)) return false;
  }
  return true;
}

uint ccstrlen(const struct ccstring* self) {
  return (self->flag == 0xFF ? self->len : self->flag);
}

void ccstrisempty(struct ccstring* self) {
  return ccstrlen(self) == 0;
}

uint ccstrcap(const struct ccstring* self) {
  if (self->flag == 0xFF) {
    struct ccheap* heap = &self->heap;
    return (heap->start < heap->beyond) ? (heap->beyond - heap->start - 1) : 0;
  }
  return CCSTRING_STATIC_CHARS;
}

const byte* ccstocstr(const struct ccstring* self) {
  return (self->flag == 0xFF ? self->heap.start : (const byte*)self);
}

struct ccfrom ccfroms(const struct ccstring* s) {
  return ccfrom(ccstocstr(s), ccstrlen(s));
}

byte* llstrenlarge(struct ccstring* self, uint bytes) {
  if (ccstrcap(self) < bytes) {
    uint len = 2 * ccstrlen(self); /* 1-byte for zero terminal */
    bytes = len >= (bytes+1) ? len : (bytes+1);
    if (self->flag == 0xFF) {
      return (ccrelloc(&self->heap, bytes) ? self->heap.start : 0;
    }
    if ((self->heap = ccallocfrom(bytes, ccfroms(self))).start == 0) {
      return 0;
    }
    self->len = self->flag;
    self->flag = 0xFF;
    return self->heap.start;
  }
  return (byte*)ccstocstr(self);
}

struct ccstring* llstrsetlen(struct ccstring* self, uint len) {
  (self->flag == 0xFF) ? (self->len = len) : (self->flag = (byte)len);
  return self;
}

struct ccstring* ccstrsets(struct ccstring* self, struct ccfrom s) {
  uint n = 0;
  if (from.start >= from.beyond) return self;
  n = from.beyond - from.start;
  if (!llstrenlarge(self, n) || ccstrcap(self) < n) {
    ccloge("ccstrsets too large %s", ccutos(n));
    return self;
  }
  if (self->flag == 0xFF) {
    self->len = cccopy(from, self->heap.start);
    *(self->heap.start + self->len) = 0;
  } else {
    self->flag = (byte)cccopy(from, (byte*)self);
    *(((byte*)self) + self->flag) = 0;
  }
  return self;
}

struct ccstring* ccstrsetsf(struct ccstring* self, struct ccfrom s, int fmt) {
  return llsfmt(&s, fmt, ccstrsets, self);
}

struct ccstring* ccstrsetu(struct ccstring* self, uint a) {
  return ccstrsetuf(self, a, 0);
}

struct ccstring* ccstrsetuf(struct ccstring* self, uint a, int fmt) {
  return llstrsetlen(self, llutos(a, fmt, llstrenlarge(self, CCSTRING_STATIC_CHARS)));
}

struct ccstring* ccstrseti(struct ccstring* self, _int a) {
  return ccstrsetif(self, a, 0);
}

struct ccstring* ccstrsetif(struct ccstring* self, _int a, int fmt) {
  return llstrsetlen(self, llutos(a < 0 ? (-a) : a, (a < 0 ? (fmt | LLNSIGN) : fmt), llstrenlarge(self, CCSTRING_STATIC_CHARS)));
}

struct ccstring* llstradds(struct ccstring* self, struct ccstring* s) {
  return ccstradds(self, ccfromstr(s));
}

struct ccstring* ccstradds(struct ccstring* self, struct ccfrom s) {
  uint nstr = ccstrlen(self), total = 0;
  if (from.start <= from.beyond) return self;
  total = nstr + (from.start - from.beyond);
  if (!llstrenlarge(self, total) || ccstrcap(self) < total) {
    ccloge("ccstradd too large %s", ccutos(total));
    return self;
  }
  if (self->flag == 0xFF) {
    self->len = nstr + cccopy(from, self->heap.start + nstr);
    *(self->heap.start + self->len) = 0;
  } else {
    self->flag = (byte)(nstr + cccopy(from, ((byte*)self) + nstr));
    *(((byte*)self) + self->falg) = 0;
  }
  return self;
}

struct ccstring* ccstraddsf(struct ccstring* self, struct ccfrom s, int fmt) {
  return llsfmt(&s, fmt, ccstradds, self);
}

struct ccstring* ccstraddu(struct ccstring* self, uint a) {
  return ccstradduf(self, a, 0);
}

struct ccstring* ccstradduf(struct ccstring* self, uint a, int fmt) {
  return llstradds(self, &ccstrfromuf(a, fmt));
}

struct ccstring* ccstraddi(struct ccstring* self, _int a) {
  return ccstraddif(self, a, 0);
}

struct ccstring* ccstraddif(struct ccstring* self, _int a, int fmt) {
  return llstradds(self, &ccstrfromif(a, fmt);
}

/* unicode */
CORE_API uint cccharstride(byte ch);
CORE_API uint ccwcharstride(ushort ch);
CORE_API bool ccstrzto16z(struct ccfrom from, struct ccdest to);
/* file operation */
CORE_API struct ccfile ccopenwrite(struct ccfrom filename);
CORE_API struct ccfile ccopenappend(struct ccfrom filename);
CORE_API struct ccfile ccopenread(struct ccfrom filename);
CORE_API uint ccwritefile(struct ccfile* self, struct ccfrom from);
CORE_API uint ccwriteline(struct ccfile* self, struct ccfrom from);
CORE_API struct ccstrz ccreadentirefile(struct ccfrom filename);
CORE_API void ccreadfile(struct ccfile* self, uint n, struct ccstrz* out);
CORE_API void ccreadline(struct ccfile* self, struct ccstrz* out);
CORE_API bool ccredirect(struct ccfrom filename);
CORE_API bool ccisopened(struct ccfile* self);
CORE_API void ccflushfile(struct ccfile* self);
CORE_API void ccclosefile(struct ccfile* self);
CORE_API uint ccgetfilesize(struct ccfrom filename); /* platform-specific */
CORE_API struct ccattr ccgetfileattr(struct ccfrom filename); /* platform-specific */

union ccfloat {
  double f;
  uint u;
};

struct ccthread {
  _int index;
  struct ccstrz s;
  void* data;
  void* func;
  void* para;
  struct cchide impl_;
};



/** 参考资料 ***
[1] UNIX环境高级编程 第6.10节 时间和日期例程 
[2] LINUX/UNIX系统编程手册 第10章 时间
[3] LINUX/UNIX系统编程手册 第23章 定时器与休眠
[4] http://man7.org/linux/man-pages/man7/time.7.html
[5] http://man7.org/linux/man-pages/man2/clock_gettime.2.html
[6] http://man7.org/linux/man-pages/man2/gettimeofday.2.html
[7] http://man7.org/linux/man-pages/man2/time.2.html
[8] http://man7.org/linux/man-pages/man3/ctime.3.html **/
struct cctime llgettime(clockid_t id) {
  struct timespec spec = {0};
  struct cctime time = {0};
  if (clock_gettime(id, &spec) != 0) {
    ccloge("clock_gettime %d %s", id, strerror(errno));
    return time;
  }
  time.secs = (_int)spec.tv_sec;
  time.ns = (medit)spec.tv_nsec;
  return time;
}
struct cctime ccgetsystime() {
  /** clock_gettime ***
  #include <sys/time.h>
  int clock_gettime(clockid_t id, struct timespec* out);
  @id为CLOCK_REALTIME时，clock_gettime提供了与time函数相同的功能，不
  过当系统支持高精度时间时，clock_gettime返回精度更高的实时系统时间；
  System-wide clock that measures real (i.e., wall-clock) time.
  Setting this clock requires appropriate privilege. This clock is
  affected by discontinuous jumps in the system time (e.g., if the
  system adaministrator manually changes the clock), and by the
  incremental adjustments performed by adjtime and NTP.
  *** gettimeofday ***
  #include <sys/time.h>
  int gettimeofday(struct timeval* tp, void* tz);
  @@SUSv4指定该函数已被弃用，但一些程序仍然使用这个函数，因为与time
  相比，gettimeofday提供了跟高的时间精度（微妙级）；
  @tz的唯一合法值是NULL，其他值将产生不确定的结果，某些平台支持用tz
  说明时区，但这完全依赖实现，SUS对此并没有定义
  @tp会保存获取到的系统时间，tp->tv_secs是time_t类型值，是相对于1970
  年1月1日00:00:00+0000以来的秒数，调用gmtime可以将time_t类型值转换成
  UTC标准日历时间，而调用localtime可转换成本地日历时间 **/
  return llgettime(CLOCK_REALTIME);
}
static struct ccdate llgetsysdate(time_t utcsecs, int tz) {

}
struct ccdate ccgetsysdate(int tz) {
  struct cctime utc = ccgetsystime();
  return llgetsysdate(utc.secs, tz);
}
struct ccdate ccgetsysdatefrom(struct cctime* utc, int tz) {
  return llgetsysdate(utc->secs, tz);
}
struct cctime ccgetboottime() {
  /** CLOCK_MONOTONIC/CLOCK_BOOTTIME ***
  CLOCK_MONOTONIC cannot be set and represents monotonic time
  since some unspecified starting point. This clock is not
  affected by discontinuous jumps in the system time, but is
  affected by the incremental adjustments performed by adjtime
  and NTP.
  CLOCK_BOOTTIME (since Linux 2.6.39, Linux-specific) is
  identical to CLOCK_MONOTONIC, except it also includes any time
  that the system is suspended. This allows applications to get
  a suspend-aware monotonic clock without having to deal with
  the complications of CLOCK_REALTIME, which may have discontinuities
  if the time is changed using settimeofday or similar. **/
  clockid_t id = CLOCK_MONOTONIC;
#ifdef CC_OS_LINUX
  id = CLOCK_BOOTTIME;
#endif
  return llgettime(id);
}
struct cctime ccgetthredtime() {
  return llgettime(CLOCK_THREAD_CPUTIME_ID);
}
struct cctime ccgetprocesstime() {
  return llgettime(CLOCK_PROCESS_CPUTIME_ID);
}
static _int llgetres(clockid_t id) {
  struct timespec spec = {0};
  if (clock_getres(id, &spec) != 0) {
    ccloge("clock_getres %s", strerror(errno));
    return 0;
  }
  return (_int)spec.tv_sec*1000000000LL + (_int)spec.tv_nsec;
}
_int ccgetsres() {
  return llgetres(CLOCK_REALTIME);
}
_int ccgetbres() {
  clockid_t id = CLOCK_MONOTONIC;
#ifdef CC_OS_LINUX
  id = CLOCK_BOOTTIME;
#endif
  return llgetres(id);
}
_int ccgettres() {
  return llgetres(CLOCK_THREAD_CPUTIME_ID);
}
_int ccgetpres() {
  return llgetres(CLOCK_PROCESS_CPUTIME_ID);
}


struct ccarray {
  struct ccheap heap;
  _int nelem;
  _int esize;
};

void ccarrayinit(struct ccarray* self, _int , _int ) {
  cczero(self, sizeof(struct ccarray));
  if (size <= 0) return;
  
}

bool ccarrayadd(struct ccarray* self, const void* elem) {
  
} 

bool ccarrayins(struct ccarray* self, _int i, const void* elem) {

}

bool ccarraydel(struct ccarray* self, _int i) {

}


/** poll - wait for some event on a fd **
#include <poll.h>
int poll(struct pollfd* fds, nfds_t nfds, int timeout);
It waits for one of a set of fds to become ready to perform I/O.
The set of fds to be monitored is specified in the fds, which is
an array of pollfd structures.
struct pollfd {
  int fd; // file descriptor
  short events; // requested events
  short revents; // returned events
};

*/


/* string */
static uint ccuint2strz(uint a, byte sign, void* to, uint minlen) {
  /* 64-bit unsigned int max value 18446744073709552046
     it is 20 characters, can be contained in struct ccstrz staticly */
  byte bf[32] = {0};
  byte* p = bf;
  uint len = 0;
  if (a == 0) *p++ = '0';
  while (a) {
    *p++ = (a % 10) + '0';
    a /= 10;
  }
  len = p - bf;
  minlen = minlen > MAX_STACKSTR_CHARS ? MAX_STACKSTR_CHARS : minlen;
  if (sign == '-' || sign == '+' || sign == ' ') minlen = minlen ? minlen-1 : 0;
  while (len < minlen) { bf[len++] = '0'; }
  if (sign == '-' || sign == '+' || sign == ' ') bf[len++] = sign;
  bf[len] = 0;
  cccopyr(ccfrom(bf, len), to);
  *(((byte*)to) + len) = 0;
  return len;
}
static uint ccreal2strz(double a, void* to) {
  byte bf[80] = {0};
  byte* p = bf;
  bool negative = false;
  uint exponent = 0;
  uint fraction = 0;
  uint len = 0;
  struct ccfloat d;
  d.f = a;
  negative = (d.u & 0x8000000000000000) != 0;
  exponent = (d.u & 0x7FF0000000000000) >> 52;
  fraction = (d.u & 0x000FFFFFFFFFFFFF);
  if (exponent == 0 && fraction == 0) {
    if (negative) *p++ = '-';
    *p++ = '0'; *p++ = '.'; *p++ = '0';
  } else if (exponent == 0x00000000000007FF) {
    if (fraction == 0) {
      if (negative) *p++ = '-';
      *p++ = 'I'; *p++ = 'n'; *p++ = 'f'; *p++ = 'i'; *p++ = 'n'; *p++ = 'i'; *p++ = 't'; *p++ = 'y';
    } else {
      *p++ = 'N'; *p++ = 'a'; *p++ = 'N';
    }
  } else {
    int res = sprintf((char*)bf, "%f", a);
    if (res > 0 && res < MAX_STACKSTR_CHARS+1) {
      p += res;
    } else {
      *p++ = '0'; *p++ = '.'; *p++ = '0';
      ccloge("real2strz convert failed %f", a);
    }
  }
  len = p - bf;
  cccopy(ccfrom(bf, len+1), to);
  return len;
}
struct ccstrz* ccdefaultstrz() {
  return &(ccthreadself()->s);
}
const byte* ccitos(_int a) {
  return ccstrzcstr(ccstrzreseti(ccdefaultstrz(), a));
}
const byte* ccsitos(const void* s, _int a) {
  return ccstrzcstr(ccstrzresetsi(ccdefaultstrz(), s, a));
}
const byte* ccistos(_int a, const void* s) {
  return ccstrzcstr(ccstrzresetis(ccdefaultstrz(), a, s));
}
const byte* ccisitos(_int a, const void* s, _int b) {
  return ccstrzcstr(ccstrzresetisi(ccdefaultstrz(), a, s, b));
}
const byte* ccisisitos(_int a, const void* s, _int b, const void* sb, _int c) {
  return ccstrzcstr(ccstrzresetisisi(ccdefaultstrz(), a, s, b, sb, c));
}
const byte* ccutos(uint a) {
  return ccstrzcstr(ccstrzresetu(ccdefaultstrz(), a));
}
const byte* ccsutos(const void* s, uint a) {
  return ccstrzcstr(ccstrzresetsu(ccdefaultstrz(), s, a));
}
const byte* ccustos(uint a, const void* s) {
  return ccstrzcstr(ccstrzresetus(ccdefaultstrz(), a, s));
}
const byte* ccusutos(uint a, const void* s, uint b) {
  return ccstrzcstr(ccstrzresetusu(ccdefaultstrz(), a, s, b));
}
const byte* ccususutos(uint a, const void* s, uint b, const void* sb, uint c) {
  return ccstrzcstr(ccstrzresetususu(ccdefaultstrz(), a, s, b, sb, c));
}
const byte* ccftos(double a) {
  return ccstrzcstr(ccstrzresetf(ccdefaultstrz(), a));
}
const byte* ccdatetos(const struct ccdate* date) {
  struct ccstrz* self = ccstrzresetum(ccdefaultstrz(), date->year, 2);
  ccstrzaddum(self, date->month, 2);
  ccstrzaddum(self, date->day, 2);
  ccstrzadds(self, " ");
  ccstrzaddu(self, date->wday);
  ccstrzadds(self, " ");
  ccstrzaddum(self, date->hour, 2);
  ccstrzaddum(self, date->min, 2);
  ccstrzaddum(self, date->sec, 2);
  return ccstrzcstr(self);
}
struct ccstrz ccemptystrz() {
  struct ccstrz self;
  cczero(&self, sizeof(struct ccstrz));
  return self;
}
struct ccstrz ccstrzfrom(struct ccfrom from) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzreset(&self, from);
}
struct ccstrz ccstrzfroms(const void* s) {
  return ccstrzfrom(ccfroms(s));
}
struct ccstrz ccstrzfromi(_int a) {
  struct ccstrz self;
  uint len = (a < 0) ? ccuint2strz((uint)(-a), '-', &self, 0) : ccuint2strz((uint)a, 0, &self, 0);
  ((struct string*)&self)->flag = (byte)len;
  return self;
}
struct ccstrz ccstrzfromim(_int a, uint minlen) {
  struct ccstrz self;
  uint len = (a < 0) ? ccuint2strz((uint)(-a), '-', &self, minlen) : ccuint2strz((uint)a, 0, &self, minlen);
  ((struct string*)&self)->flag = (byte)len;
  return self;
}
struct ccstrz ccstrzfromis(_int a, const void* s) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzresetis(&self, a, s);
}
struct ccstrz ccstrzfromsi(const void* s, _int a) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzresetsi(&self, s, a);
}
struct ccstrz ccstrzfromisi(_int a, const void* s, _int b) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzresetisi(&self, a, s, b);
}
struct ccstrz ccstrzfromisisi(_int a, const void* s, _int b, const void* sb, _int c) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzresetisisi(&self, a, s, b, sb, c);
}
struct ccstrz ccstrzfromu(uint a) {
  struct ccstrz self;
  uint len = ccuint2strz(a, 0, &self, 0);
  ((struct string*)&self)->flag = (byte)len;
  return self;
}
struct ccstrz ccstrzfromum(uint a, uint minlen) {
  struct ccstrz self;
  uint len = ccuint2strz(a, 0, &self, minlen);
  ((struct string*)&self)->flag = (byte)len;
  return self;
}
struct ccstrz ccstrzfromus(uint a, const void* s) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzresetus(&self, a, s);
}
struct ccstrz ccstrzfromsu(const void* s, uint a) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzresetsu(&self, s, a);
}
struct ccstrz ccstrzfromusu(uint a, const void* s, uint b) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzresetusu(&self, a, s, b);
}
struct ccstrz ccstrzfromususu(uint a, const void* s, uint b, const void* sb, uint c) {
  struct ccstrz self;
  ((struct string*)&self)->flag = 0;
  return *ccstrzresetususu(&self, a, s, b, sb, c);
}
struct ccstrz ccstrzfromf(double a) {
  struct ccstrz self;
  uint len = ccreal2strz(a, &self);
  ((struct string*)&self)->flag = (byte)len;
  return self;
}
uint ccstrzlen(const struct ccstrz* self) {
  return ((struct string*)self)->flag == 0xFF ? ((struct string*)self)->len : (uint)(((struct string*)self)->flag);
}
uint ccstrzmax(const struct ccstrz* self) {
  return ((struct string*)self)->flag == 0xFF ? ((struct string*)self)->heap.len - 1 : (uint)(sizeof(struct ccstrz)-2);
}
struct ccfrom ccstrzget(const struct ccstrz* self) {
  return ccfrom(ccstrzcstr(self), ccstrzlen(self));
}
const byte* ccstrzcstr(const struct ccstrz* self) {
  if (((struct string*)self)->flag == 0xFF) return ((struct string*)self)->heap.buffer;
  return (const byte*)self;
}
void ccstrzfree(struct ccstrz* self) {
  if (((struct string*)self)->flag == 0xFF) { ccfree(&((struct string*)self)->heap); ((struct string*)self)->len = 0; }
  ((struct string*)self)->flag = 0;
}
bool ccisemptystrz(const struct ccstrz* self) {
  return ccstrzlen(self) == 0;
}
bool ccstrzequal(struct ccfrom a, struct ccfrom b) {
  uint i = 0;
  if (a.len != b.len) return false;
  for (; i < a.len; ++i) {
    if (((const byte*)a.start)[i] != ((const byte*)b.start)[i]) return false;
  }
  return true;
}
static byte* ccstrzenlarge(struct ccstrz* self, uint n) {
  if (n > ccstrzmax(self)) {
    if (((struct string*)self)->flag == 0xFF) {
      ccrelloc(&((struct string*)self)->heap, n);
    } else {
      ((struct string*)self)->heap = ccallocs(n, ccfrom(self, ccstrzmax(self)));
      ((struct string*)self)->flag = 0xFF;
    }
  }
  return (byte*)ccstrzcstr(self);
}
static void ccstrzsetlen(struct ccstrz* self, uint len) {
  if (((struct string*)self)->flag == 0xFF) ((struct string*)self)->len = len;
  else ((struct string*)self)->flag = (byte)len;
}
struct ccstrz* ccstrzreset(struct ccstrz* self, struct ccfrom s) {
  byte* to = ccstrzenlarge(self, s.len+1);
  cccopy(s, to);
  *(to + s.len) = 0;
  ccstrzsetlen(self, s.len);
  return self;
}
struct ccstrz* ccstrzresets(struct ccstrz* self, const void* s) {
  return ccstrzreset(self, ccfroms(s));
}
struct ccstrz* ccstrzreseti(struct ccstrz* self, _int a) {
  byte* p = ccstrzenlarge(self, MAX_STACKSTR_CHARS);
  ccstrzsetlen(self, (a < 0) ? ccuint2strz((uint)(-a), '-', p, 0) : ccuint2strz((uint)a, 0, p, 0));
  return self;
}
struct ccstrz* ccstrzresetim(struct ccstrz* self, _int a, uint minlen) {
  byte* p = ccstrzenlarge(self, MAX_STACKSTR_CHARS);
  ccstrzsetlen(self, (a < 0) ? ccuint2strz((uint)(-a), '-', p, minlen) : ccuint2strz((uint)a, 0, p, minlen));
  return self;
}
struct ccstrz* ccstrzresetis(struct ccstrz* self, _int a, const void* s) {
  ccstrzreseti(self, a);
  ccstrzadds(self, s);
  return self;
}
struct ccstrz* ccstrzresetsi(struct ccstrz* self, const void* s, _int a) {
  ccstrzresets(self, s);
  ccstrzaddi(self, a);
  return self;
}
struct ccstrz* ccstrzresetisi(struct ccstrz* self, _int a, const void* s, _int b) {
  ccstrzresetis(self, a, s);
  ccstrzaddi(self, b);
  return self;
}
struct ccstrz* ccstrzresetisisi(struct ccstrz* self, _int a, const void* s, _int b, const void* sb, _int c) {
  ccstrzresetisi(self, a, s, b);
  ccstrzadds(self, sb);
  ccstrzaddi(self, c);
  return self;
}
struct ccstrz* ccstrzresetu(struct ccstrz* self, uint a) {
  ccstrzsetlen(self, ccuint2strz(a, 0, ccstrzenlarge(self, MAX_STACKSTR_CHARS), 0));
  return self;
}
struct ccstrz* ccstrzresetum(struct ccstrz* self, uint a, uint minlen) {
  ccstrzsetlen(self, ccuint2strz(a, 0, ccstrzenlarge(self, MAX_STACKSTR_CHARS), minlen));
  return self;
}
struct ccstrz* ccstrzresetus(struct ccstrz* self, uint a, const void* s) {
  ccstrzresetu(self, a);
  ccstrzadds(self, s);
  return self;
}
struct ccstrz* ccstrzresetsu(struct ccstrz* self, const void* s, uint a) {
  ccstrzresets(self, s);
  ccstrzaddi(self, a);
  return self;
}
struct ccstrz* ccstrzresetusu(struct ccstrz* self, uint a, const void* s, uint b) {
  ccstrzresetus(self, a, s);
  ccstrzaddu(self, b);
  return self;
}
struct ccstrz* ccstrzresetususu(struct ccstrz* self, uint a, const void* s, uint b, const void* sb, uint c) {
  ccstrzresetusu(self, a, s, b);
  ccstrzadds(self, sb);
  ccstrzaddu(self, c);
  return self;
}
struct ccstrz* ccstrzresetf(struct ccstrz* self, double a) {
  ccstrzsetlen(self, ccreal2strz(a, ccstrzenlarge(self, MAX_STACKSTR_CHARS)));
  return self;
}
struct ccstrz* ccstrzaddbyte(struct ccstrz* self, byte a) {
  return ccstrzadd(self, ccfrom(&a, 1));
}
struct ccstrz* ccstrzadd(struct ccstrz* self, struct ccfrom from) {
  uint len = ccstrzlen(self);
  struct string* st = (struct string*)self;
  uint bfsz = 0;
  if (len + from.len <= ccstrzmax(self)) {
    if (st->flag == 0xFF) {
      cccopy(from, ((byte*)st->heap.buffer) + len);
      *(((byte*)st->heap.buffer) + len + from.len) = 0;
      st->len = len + from.len;
    } else {
      cccopy(from, ((byte*)st) + len);
      *(((byte*)st) + len + from.len) = 0;
      st->flag = (byte)(len + from.len);
    }
    return self;
  }
  bfsz = len + from.len + 1;
  bfsz = len*2 > bfsz ? len*2 : bfsz;
  if (st->flag == 0xFF) {
    ccrelloc(&st->heap, bfsz);
  } else {
    st->heap = ccallocs(bfsz, ccfrom(st, len));
  }
  cccopy(from, ((byte*)st->heap.buffer) + len);
  *(((byte*)st->heap.buffer) + len + from.len) = 0;
  st->len = len + from.len;
  st->flag = 0xFF;
  return self;
}
struct ccstrz* ccstrzadds(struct ccstrz* self, const void* s) {
  ccstrzadd(self, ccfroms(s));
  return self;
}
struct ccstrz* ccstrzaddi(struct ccstrz* self, _int a) {
  struct ccstrz s = ccstrzfromi(a);
  struct ccstrz* p = ccstrzadd(self, ccstrzget(&s));
  ccstrzfree(&s);
  return p;
}
struct ccstrz* ccstrzaddim(struct ccstrz* self, _int a, uint minlen) {
  struct ccstrz s = ccstrzfromim(a, minlen);
  struct ccstrz* p = ccstrzadd(self, ccstrzget(&s));
  ccstrzfree(&s);
  return p;
}
struct ccstrz* ccstrzaddu(struct ccstrz* self, uint a) {
  struct ccstrz s = ccstrzfromu(a);
  struct ccstrz* p = ccstrzadd(self, ccstrzget(&s));
  ccstrzfree(&s);
  return p;
}
struct ccstrz* ccstrzaddum(struct ccstrz* self, uint a, uint minlen) {
  struct ccstrz s = ccstrzfromum(a, minlen);
  struct ccstrz* p = ccstrzadd(self, ccstrzget(&s));
  ccstrzfree(&s);
  return p;
}
struct ccstrz* ccstrzaddf(struct ccstrz* self, double a) {
  struct ccstrz s = ccstrzfromf(a);
  struct ccstrz* p = ccstrzadd(self, ccstrzget(&s));
  ccstrzfree(&s);
  return p;
}






/** unicode **/

static const byte struct cccharstride_a[] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 64 */
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,0,0
}; /* github.com/dlang/druntime/blob/master/src/rt/util/utf.d */

sint cccharstride(byte ch) {
  return struct cccharstride_a[ch];
} /* return 0 means s[i] is not the start of utf8 sequence */

sint ccwcharstride(ushort ch) {
  return 1 + (ch >= 0xD800 && ch <= 0xDBFF);
}

static umedit cccharat(struct ccfrom from, sint* i) {
  byte ch = 0;
  byte n = 0;
  umedit dest = 0;
  uint idx = 0;
  if (*i >= from.len) return 0;
  ch = *(((const byte*)from.start) + *i);
  if (!(ch & 0x80)) {  /* 0xxxxxxx */
    *i = *i + 1;       /* 110xxxxx 10xxxxxx */
    return (umedit)ch; /* 1110xxxx 10xxxxxx 10xxxxxx */
  }                    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
  n = 1;               /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
  for (;; ++n) {       /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if ((ch << n) & 0x80) {
      if (n < 6) continue;
      ccloge("charat fail: 1111111X %s", ccutos(*i));
      *i = *i + 1;
      return 0;
    }
    if (n == 1) {
      ccloge("charat fail: 10XXXXXX %s", ccutos(*i));
      *i = *i + 1;
      return 0;
    }
    break;
  }
  if (*i + n > from.len) {
    ccloge("charat fail: out of range %s", ccususutos(*i, "+", n, " len ", from.len));
    *i = from.len;
    return 0;
  }
  dest = (ch & ((1 << (7 - n)) - 1));
  idx = 1;
  for (; idx < n; ++idx) {
    ch = *(((const byte*)from.start) + *i + idx);
    if ((ch & 0xC0) != 0x80) {
      ccloge("charat fail: !10XXXXXX %s", ccusutos(ch, " at ", *i));
    }
    dest = (dest << 6) | (ch & 0x3F);
  }
  *i = *i + n;
  return dest;
}

static bool cctowchar(umedit c, struct ccdest dest, sint* i) {
  if (c <= 0xFFFF) {
    if (*i >= dest.len) return false;
    *(((ushort*)dest.start) + *i) = (ushort)c;
    *i = *i + 1;
    return true;
  }
  if (*i + 2 > dest.len) return false;
  *(((ushort*)dest.start) + *i) = (ushort)((((c - 0x10000) >> 10) & 0x3FF) + 0xD800);
  *(((ushort*)dest.start) + *i + 1) = (ushort)(((c - 0x10000) & 0x3FF) + 0xDC00);
  *i = *i + 2;
  return true;
}

bool ccstrzto16z(struct ccfrom from, struct ccdest to) {
  const byte* p = 0;
  ushort* dest = 0;
  sint i = 0, toidx = 0;
  if (to.len == 0) return false;
  p = (const byte*)from.start;
  dest = (ushort*)to.start;
  while (i < from.len) {
    umedit ch = p[i];
    if (ch <= 0x7F) {
      if (toidx < to.len) dest[toidx++] = p[i];
      else break;
    } else {
      ch = cccharat(from, &i);
      if (!cctowchar(ch, to, &toidx)) break;
    }
  }
  if (toidx < to.len) {
    dest[toidx] = 0;
    return true;
  }
  dest[toidx-1] = 0;
  return false;
}

/* file operation */
struct ccfile ccopenwrite(struct ccfrom filename) {
  struct ccfile f;
  f.impl_ = fopen((const char*)filename.start, "wb");
  if (!f.impl_) ccloge("openwrite fail: %s file %s", strerror(errno), filename.start);
  return f;
}
struct ccfile ccopenappend(struct ccfrom filename) {
  struct ccfile f;
  f.impl_ = fopen((const char*)filename.start, "ab");
  if (!f.impl_) ccloge("openappend fail: %s file %s", strerror(errno), filename.start);
  return f;
}
struct ccfile ccopenread(struct ccfrom filename) {
  struct ccfile f;
  f.impl_ = fopen((const char*)filename.start, "rb");
  if (!f.impl_) ccloge("openread fail: %s file %s", strerror(errno), filename.start);
  return f;
}
uint ccwritefile(struct ccfile* self, struct ccfrom from) {
  size_t count = 0;
  size_t sz = 0;
  if (!ccisopened(self) || from.len == 0) return 0;
  count = (size_t)from.len;
  sz = fwrite(from.start, 1, count, (FILE*)self->impl_);
  if (sz != count) ccloge("writeline fail: %s", strerror(errno));
  return (uint)sz;
}
uint ccwriteline(struct ccfile* self, struct ccfrom from) {
  size_t count = 0;
  size_t sz = 0;
  if (!ccisopened(self) || from.len == 0) return 0;
  count = (size_t)from.len;
  sz = fwrite(from.start, 1, count, (FILE*)self->impl_);
  if (sz == count) fwrite(CCNEWLINE, 1, CCNLSIZE, (FILE*)self->impl_);
  else ccloge("writeline fail: %s", strerror(errno));
  return (uint)sz;
}
struct ccstrz ccreadentirefile(struct ccfrom filename) {
  struct ccfile file = ccopenread(filename);
  struct ccstrz s = ccemptystrz();
  uint size = 0;
  if (!ccisopened(&file)) return s;
  size = ccgetfilesize(filename);
  ccreadfile(&file, size, &s);
  return s;
}
void ccreadfile(struct ccfile* self, uint n, struct ccstrz* out) {
  byte* p = 0;
  size_t count = 0;
  size_t sz = 0;
  if (!ccisopened(self) || n == 0) return;
  p = ccstrzenlarge(out, n);
  count = (size_t)n;
  sz = fread(p, 1, count, (FILE*)self->impl_);
  if (sz != count && (ferror((FILE*)self->impl_) || !feof((FILE*)self->impl_))) {
    ccloge("readfile fail: %s", strerror(errno));
  }
  ccstrzsetlen(out, (uint)sz);
}
void ccreadline(struct ccfile* self, struct ccstrz* out) {
  int ch = 0;
  if (!ccisopened(self)) return;
  ch = fgetc((FILE*)self->impl_);
  while (ch != '\r' && ch != '\n' && ch != EOF) {
    ccstrzaddbyte(out, (byte)ch);
    ch = fgetc((FILE*)self->impl_);
  }
  if (ch == EOF) {
    if (ferror((FILE*)self->impl_)) ccloge("readline fail: %s", strerror(errno));
    return;
  }
  if (ch == '\r') {
    ch = fgetc((FILE*)self->impl_);
    if (ch != '\n' && ch != EOF) ungetc((byte)ch, (FILE*)self->impl_);
    return;
  }
  if (ch == '\n') {
    ch = fgetc((FILE*)self->impl_);
    if (ch != '\r' && ch != EOF) ungetc((byte)ch, (FILE*)self->impl_);
  }
}
bool ccredirect(struct ccfrom filename) {
  if (!freopen((const char*)filename.start, "wb", stdout)) {
    ccloge("redirect stdout fail: %s", strerror(errno));
    return false;
  }
  if (!freopen((const char*)filename.start, "wb", stderr)) {
    ccloge("redirect stderr fail: %s", strerror(errno));
    return false;
  }
  return true;
}
bool ccisopened(struct ccfile* self) {
  return self->impl_ != 0;
}
void ccflushfile(struct ccfile* self) {
  if (!ccisopened(self)) return;
  fflush((FILE*)self->impl_);
}
void ccclosefile(struct ccfile* self) {
  if (self->impl_) {
    fclose((FILE*)self->impl_);
    self->impl_ = 0;
  }
}

/* test */
void ccthattest() {
  _int level = 0;
  struct ccattr fa;
  struct ccstrz s;
#ifdef CORE_DEBUG_ON
  cclogd("CORE_DEBUG_ON true");
#else
  cclogd("CORE_DEBUG_ON false");
#endif
  ccassert(ccisprime(0) == false);
  ccassert(ccisprime(1) == false);
  ccassert(ccisprime(2) == true);
  ccassert(ccisprime(3) == true);
  ccassert(ccisprime(5) == true);
  ccassert(ccisprime(7) == true);
  ccassert(ccisprime(CC_UMED_MAX) == false);
  ccassert(sizeof(byte) == 1);
  ccassert(sizeof(int8) == 1);
  ccassert(sizeof(sshort) == 2);
  ccassert(sizeof(ushort) == 2);
  ccassert(sizeof(medit) == 4);
  ccassert(sizeof(umedit) == 4);
  ccassert(sizeof(sint) == 8);
  ccassert(sizeof(uint) == 8);
  ccassert(ccbytemax() == 255);
  ccassert(ccint8max() == 127);
  ccassert(ccint8min() == -127-1);
  ccassert(((byte)ccint8min()) == 0x80);
  ccassert(((byte)ccint8min()) == 128);
  ccassert(ccushortmax() == 65535);
  ccassert(ccshortmax() == 32767);
  ccassert(ccshortmin() == -32767-1);
  ccassert(((ushort)ccshortmin()) == 32768);
  ccassert(((ushort)ccshortmin()) == 0x8000);
  ccassert(ccumeditmax() == 4294967295);
  ccassert(ccmeditmax() == 2147483647);
  ccassert(ccmeditmin() == -2147483647-1);
  ccassert(((umedit)ccmeditmin()) == 2147483648);
  ccassert(((umedit)ccmeditmin()) == 0x80000000);
  ccassert(ccuintmax() == 18446744073709551615ull);
  ccassert(ccintmax() == 9223372036854775807ull);
  ccassert(ccintmin() == -9223372036854775807-1);
  ccassert(((uint)ccintmin()) == 9223372036854775808ull);
  ccassert(((uint)ccintmin()) == 0x8000000000000000ull);
  ccassert(sizeof(uint) >= sizeof(size_t));
  ccassert(sizeof(double) == 8);
  ccassert(sizeof(struct ccstrz) == 32);
  ccassert(sizeof(struct string) == 32);
  ccutos(ccuintmax());
  ccassert(ccstrzlen(ccdefaultstrz()) < MAX_STACKSTR_CHARS);
  ccassert(ccstrzmax(ccdefaultstrz()) == MAX_STACKSTR_CHARS);
  cclogd("%%        %%");
  cclogd("%%f       %f", 1.01);
  cclogd("%%-f      %-f", 1.01);
  cclogd("%%+f      %+f", 1.01);
  cclogd("%% f      % f", 1.01);
  cclogd("%%0f      %0f", 1.01);
  cclogd("%%4f      %4f", 1.01);
  cclogd("%%-4f     %-4f", 1.01);
  cclogd("%%+4f     %+4f", 1.01);
  cclogd("%% 4f     % 4f", 1.01);
  cclogd("%%04f     %04f", 1.01);
  cclogd("%%.4f     %.4f", 1.01);
  cclogd("%%-.4f    %-.4f", 1.01);
  cclogd("%%+.4f    %+.4f", 1.01);
  cclogd("%% .4f    % .4f", 1.01);
  cclogd("%%0.4f    %0.4f", 1.01);
  cclogd("%%4.4f    %4.4f", 1.01);
  cclogd("%%-4.4f   %-4.4f", 1.01);
  cclogd("%%+4.4f   %+4.4f", 1.01);
  cclogd("%% 4.4f   % 4.4f", 1.01);
  cclogd("%%04.4f   %04.4f", 1.01);
  cclogd("%%12f     %12f", 1.01);
  cclogd("%%-12f    %-12f", 1.01);
  cclogd("%%+12f    %+12f", 1.01);
  cclogd("%% 12f    % 12f", 1.01);
  cclogd("%%012f    %012f", 1.01);
  cclogd("%%.12f    %.12f", 1.01);
  cclogd("%%-.12f   %-.12f", 1.01);
  cclogd("%%+.12f   %+.12f", 1.01);
  cclogd("%% .12f   % .12f", 1.01);
  cclogd("%%0.12f   %0.12f", 1.01);
  cclogd("%%4.12f   %4.12f", 1.01);
  cclogd("%%-4.12f  %-4.12f", 1.01);
  cclogd("%%+4.12f  %+4.12f", 1.01);
  cclogd("%% 4.12f  % 4.12f", 1.01);
  cclogd("%%04.12f  %04.12f", 1.01);
  cclogd("%%12.4f   %12.4f", 1.01);
  cclogd("%%-12.4f  %-12.4f", 1.01);
  cclogd("%%+12.4f  %+12.4f", 1.01);
  cclogd("%% 12.4f  % 12.4f", 1.01);
  cclogd("%%012.4f  %012.4f", 1.01);
  cclogd("%%12.12f  %12.12f", 1.01);
  cclogd("%%-12.12f %-12.12f", 1.01);
  cclogd("%%+12.12f %+12.12f", 1.01);
  cclogd("%% 12.12f % 12.12f", 1.01);
  cclogd("%%012.12f %012.12f", 1.01);
  cclogd("%%--12.4f %--12.4f", 1.01);
  cclogd("%%-+12.4f %-+12.4f", 1.01);
  cclogd("%%- 12.4f %- 12.4f", 1.01);
  cclogd("%%-012.4f %-012.4f", 1.01);
  cclogd("%%++12.4f %++12.4f", 1.01);
  cclogd("%%+-12.4f %+-12.4f", 1.01);
  cclogd("%%+ 12.4f %+ 12.4f", 1.01);
  cclogd("%%+012.4f %+012.4f", 1.01);
  cclogd("%%  12.4f %  12.4f", 1.01);
  cclogd("%% -12.4f % -12.4f", 1.01);
  cclogd("%% +12.4f % +12.4f", 1.01);
  cclogd("%% 012.4f % 012.4f", 1.01);
  cclogd("%%0012.4f %0012.4f", 1.01);
  cclogd("%%0-12.4f %0-12.4f", 1.01);
  cclogd("%%0+12.4f %0+12.4f", 1.01);
  cclogd("%%0 12.4f %0 12.4f", 1.01);
  level = ccgetloglevel();
  ccsetloglevel(0);
  ccassert(cclogd("%%        %") == 0);
  ccassert(cclogd("%%f       %") == 0);
  ccassert(cclogd("%%-f      %-") == 0);
  ccassert(cclogd("%%+f      %+") == 0);
  ccassert(cclogd("%% f      % ") == 0);
  ccassert(cclogd("%%0f      %0") == 0);
  ccassert(cclogd("%%4f      %4") == 0);
  ccassert(cclogd("%%-4f     %-4") == 0);
  ccassert(cclogd("%%+4f     %+4") == 0);
  ccassert(cclogd("%% 4f     % 4") == 0);
  ccassert(cclogd("%%04f     %04") == 0);
  ccassert(cclogd("%%.4f     %.4") == 0);
  ccassert(cclogd("%%-.4f    %-.4") == 0);
  ccassert(cclogd("%%+.4f    %+.4") == 0);
  ccassert(cclogd("%% .4f    % .4") == 0);
  ccassert(cclogd("%%0.4f    %0.4") == 0);
  ccassert(cclogd("%%4.4f    %4.4") == 0);
  ccassert(cclogd("%%-4.4f   %-4.4") == 0);
  ccassert(cclogd("%%+4.4f   %+4.4") == 0);
  ccassert(cclogd("%% 4.4f   % 4.4") == 0);
  ccassert(cclogd("%%04.4f   %04.4") == 0);
  ccassert(cclogd("%%12f     %12") == 0);
  ccassert(cclogd("%%-12f    %-12") == 0);
  ccassert(cclogd("%%+12f    %+12") == 0);
  ccassert(cclogd("%% 12f    % 12") == 0);
  ccassert(cclogd("%%012f    %012") == 0);
  ccassert(cclogd("%%.12f    %.12") == 0);
  ccassert(cclogd("%%-.12f   %-.12") == 0);
  ccassert(cclogd("%%+.12f   %+.12") == 0);
  ccassert(cclogd("%% .12f   % .12") == 0);
  ccassert(cclogd("%%0.12f   %0.12") == 0);
  ccassert(cclogd("%%4.12f   %4.12") == 0);
  ccassert(cclogd("%%-4.12f  %-4.12") == 0);
  ccassert(cclogd("%%+4.12f  %+4.12") == 0);
  ccassert(cclogd("%% 4.12f  % 4.12") == 0);
  ccassert(cclogd("%%04.12f  %04.12") == 0);
  ccassert(cclogd("%%12.4f   %12.4") == 0);
  ccassert(cclogd("%%-12.4f  %-12.4") == 0);
  ccassert(cclogd("%%+12.4f  %+12.4") == 0);
  ccassert(cclogd("%% 12.4f  % 12.4") == 0);
  ccassert(cclogd("%%012.4f  %012.4") == 0);
  ccassert(cclogd("%%12.12f  %12.12") == 0);
  ccassert(cclogd("%%-12.12f %-12.12") == 0);
  ccassert(cclogd("%%+12.12f %+12.12") == 0);
  ccassert(cclogd("%% 12.12f % 12.12") == 0);
  ccassert(cclogd("%%012.12f %012.12") == 0);
  ccassert(cclogd("%%--12.4f %--12.4") == 0);
  ccassert(cclogd("%%-+12.4f %-+12.4") == 0);
  ccassert(cclogd("%%- 12.4f %- 12.4") == 0);
  ccassert(cclogd("%%-012.4f %-012.4") == 0);
  ccassert(cclogd("%%++12.4f %++12.4") == 0);
  ccassert(cclogd("%%+-12.4f %+-12.4") == 0);
  ccassert(cclogd("%%+ 12.4f %+ 12.4") == 0);
  ccassert(cclogd("%%+012.4f %+012.4") == 0);
  ccassert(cclogd("%%  12.4f %  12.4") == 0);
  ccassert(cclogd("%% -12.4f % -12.4") == 0);
  ccassert(cclogd("%% +12.4f % +12.4") == 0);
  ccassert(cclogd("%% 012.4f % 012.4") == 0);
  ccassert(cclogd("%%0012.4f %0012.4") == 0);
  ccassert(cclogd("%%0-12.4f %0-12.4") == 0);
  ccassert(cclogd("%%0+12.4f %0+12.4") == 0);
  ccassert(cclogd("%%0 12.4f %0 12.4") == 0);
  ccassert(cclogd("%%        %z") == 0);
  ccassert(cclogd("%%f       %z") == 0);
  ccassert(cclogd("%%-f      %-z") == 0);
  ccassert(cclogd("%%+f      %+z") == 0);
  ccassert(cclogd("%% f      % z") == 0);
  ccassert(cclogd("%%0f      %0z") == 0);
  ccassert(cclogd("%%4f      %4z") == 0);
  ccassert(cclogd("%%-4f     %-4z") == 0);
  ccassert(cclogd("%%+4f     %+4z") == 0);
  ccassert(cclogd("%% 4f     % 4z") == 0);
  ccassert(cclogd("%%04f     %04z") == 0);
  ccassert(cclogd("%%.4f     %.4z") == 0);
  ccassert(cclogd("%%-.4f    %-.4z") == 0);
  ccassert(cclogd("%%+.4f    %+.4z") == 0);
  ccassert(cclogd("%% .4f    % .4z") == 0);
  ccassert(cclogd("%%0.4f    %0.4z") == 0);
  ccassert(cclogd("%%4.4f    %4.4z") == 0);
  ccassert(cclogd("%%-4.4f   %-4.4z") == 0);
  ccassert(cclogd("%%+4.4f   %+4.4z") == 0);
  ccassert(cclogd("%% 4.4f   % 4.4z") == 0);
  ccassert(cclogd("%%04.4f   %04.4z") == 0);
  ccassert(cclogd("%%12f     %12z") == 0);
  ccassert(cclogd("%%-12f    %-12z") == 0);
  ccassert(cclogd("%%+12f    %+12z") == 0);
  ccassert(cclogd("%% 12f    % 12z") == 0);
  ccassert(cclogd("%%012f    %012z") == 0);
  ccassert(cclogd("%%.12f    %.12z") == 0);
  ccassert(cclogd("%%-.12f   %-.12z") == 0);
  ccassert(cclogd("%%+.12f   %+.12z") == 0);
  ccassert(cclogd("%% .12f   % .12z") == 0);
  ccassert(cclogd("%%0.12f   %0.12z") == 0);
  ccassert(cclogd("%%4.12f   %4.12z") == 0);
  ccassert(cclogd("%%-4.12f  %-4.12z") == 0);
  ccassert(cclogd("%%+4.12f  %+4.12z") == 0);
  ccassert(cclogd("%% 4.12f  % 4.12z") == 0);
  ccassert(cclogd("%%04.12f  %04.12z") == 0);
  ccassert(cclogd("%%12.4f   %12.4z") == 0);
  ccassert(cclogd("%%-12.4f  %-12.4z") == 0);
  ccassert(cclogd("%%+12.4f  %+12.4z") == 0);
  ccassert(cclogd("%% 12.4f  % 12.4z") == 0);
  ccassert(cclogd("%%012.4f  %012.4z") == 0);
  ccassert(cclogd("%%12.12f  %12.12z") == 0);
  ccassert(cclogd("%%-12.12f %-12.12z") == 0);
  ccassert(cclogd("%%+12.12f %+12.12z") == 0);
  ccassert(cclogd("%% 12.12f % 12.12z") == 0);
  ccassert(cclogd("%%012.12f %012.12z") == 0);
  ccassert(cclogd("%%--12.4f %--12.4z") == 0);
  ccassert(cclogd("%%-+12.4f %-+12.4z") == 0);
  ccassert(cclogd("%%- 12.4f %- 12.4z") == 0);
  ccassert(cclogd("%%-012.4f %-012.4z") == 0);
  ccassert(cclogd("%%++12.4f %++12.4z") == 0);
  ccassert(cclogd("%%+-12.4f %+-12.4z") == 0);
  ccassert(cclogd("%%+ 12.4f %+ 12.4z") == 0);
  ccassert(cclogd("%%+012.4f %+012.4z") == 0);
  ccassert(cclogd("%%  12.4f %  12.4z") == 0);
  ccassert(cclogd("%% -12.4f % -12.4z") == 0);
  ccassert(cclogd("%% +12.4f % +12.4z") == 0);
  ccassert(cclogd("%% 012.4f % 012.4z") == 0);
  ccassert(cclogd("%%0012.4f %0012.4z") == 0);
  ccassert(cclogd("%%0-12.4f %0-12.4z") == 0);
  ccassert(cclogd("%%0+12.4f %0+12.4z") == 0);
  ccassert(cclogd("%%0 12.4f %0 12.4z") == 0);
  ccassert(cclogd("%%123.12f %123.12f") == 0);
  ccassert(cclogd("%%123.f   %123.f") == 0);
  ccassert(cclogd("%%123.123f %123.123f") == 0);
  ccsetloglevel(level);
  cclogd("autoconf.c size %s", ccutos(ccgetfilesize(ccfroms("autoconf.c"))));
  fa = ccgetfileattr(ccfroms("autoconf.c"));
  cclogd("autoconf.c attr fsize %s", ccutos(fa.fsize));
  cclogd("autoconf.c attr ctime %s", ccutos(fa.ctime));
  cclogd("autoconf.c attr atime %s", ccutos(fa.atime));
  cclogd("autoconf.c attr mtime %s", ccutos(fa.mtime));
  cclogd("autoconf.c attr cdate %s", ccdatetos(&fa.cdate));
  cclogd("autoconf.c attr adate %s", ccdatetos(&fa.adate));
  cclogd("autoconf.c attr mdate %s", ccdatetos(&fa.mdate));
  s = ccstrzfromum(1024, 8);
  ccassert(ccstrzequal(ccstrzget(&s), ccfroms("00001024")));
  cclogd("struct ccstrz %s", ccstrzcstr(&s));
  cclogd("struct string len %s", ccutos(ccstrzlen(&s)));
  cclogd("max. chars %s", ccutos(ccstrzmax(&s)));
  ccstrzaddum(&s, 2048, 8);
  ccassert(ccstrzequal(ccstrzget(&s), ccfroms("0000102400002048")));
  cclogd("struct ccstrz %s", ccstrzcstr(&s));
  cclogd("struct string len %s", ccutos(ccstrzlen(&s)));
  cclogd("max. chars %s", ccutos(ccstrzmax(&s)));
  ccstrzaddum(&s, 4096, 8);
  ccassert(ccstrzequal(ccstrzget(&s), ccfroms("000010240000204800004096")));
  cclogd("struct ccstrz %s", ccstrzcstr(&s));
  cclogd("struct string len %s", ccutos(ccstrzlen(&s)));
  cclogd("max. chars %s", ccutos(ccstrzmax(&s)));
  ccstrzaddu(&s, 8192);
  ccassert(ccstrzequal(ccstrzget(&s), ccfroms("0000102400002048000040968192")));
  cclogd("struct ccstrz %s", ccstrzcstr(&s));
  cclogd("struct string len %s", ccutos(ccstrzlen(&s)));
  cclogd("max. chars %s", ccutos(ccstrzmax(&s)));
  ccstrzaddu(&s, 0);
  ccassert(ccstrzequal(ccstrzget(&s), ccfroms("00001024000020480000409681920")));
  cclogd("struct ccstrz %s", ccstrzcstr(&s));
  cclogd("struct string len %s", ccutos(ccstrzlen(&s)));
  cclogd("max. chars %s", ccutos(ccstrzmax(&s)));
  ccstrzaddu(&s, 1);
  ccassert(ccstrzequal(ccstrzget(&s), ccfroms("000010240000204800004096819201")));
  cclogd("struct ccstrz %s", ccstrzcstr(&s));
  cclogd("struct string len %s", ccutos(ccstrzlen(&s)));
  cclogd("max. chars %s", ccutos(ccstrzmax(&s)));
  ccstrzaddu(&s, 2);
  ccassert(ccstrzequal(ccstrzget(&s), ccfroms("0000102400002048000040968192012")));
  cclogd("struct ccstrz %s", ccstrzcstr(&s));
  cclogd("struct string len %s", ccutos(ccstrzlen(&s)));
  cclogd("max. chars %s", ccutos(ccstrzmax(&s)));
  ccstrzaddu(&s, 3);
  ccassert(ccstrzequal(ccstrzget(&s), ccfroms("00001024000020480000409681920123")));
  cclogd("struct ccstrz %s", ccstrzcstr(&s));
  cclogd("struct string len %s", ccutos(ccstrzlen(&s)));
  cclogd("max. chars %s", ccutos(ccstrzmax(&s)));
  ccstrzfree(&s);
}



typedef struct {
  void* (*start)(void*);
  void* para;
  corethread_t* thread;
} threadargs_t;
#define MAX_THREADPOOL_SIZE 128
static corethread_t threadpool[MAX_THREADPOOL_SIZE];
static uint threadpoolindex = 0;
static pthread_key_t pthread_tls_key;
static bool tlskey_created = false;
static pthread_t themainthread;
static corethread_t* gettopthread() {
  corethread_t* p = 0;
  if (threadpoolindex >= MAX_THREADPOOL_SIZE) {
    core_loge("thread reach limit %s", core_utos(MAX_THREADPOOL_SIZE));
    return 0;
  }
  p = threadpool + threadpoolindex++;
  p->index = 0;
  p->func = p->para = 0;
  core_zero(&p->impl_, sizeof(corehide_t));
  return p;
}
static void freetopthread() {
  if (threadpoolindex) threadpoolindex -= 1;
}
static pthread_t* getpthreadaddr(corethread_t* self) {
  return (pthread_t*)&(self->impl_);
}
static pthread_key_t getthreadkey() {
  if (!tlskey_created) core_loge("getthreadkey unavailable");
  return pthread_tls_key;
}
static void createthreadkey() {
  int errnum = 0;
  if (tlskey_created) return;
  errnum = pthread_key_create(&pthread_tls_key, 0);
  if (errnum == 0) { tlskey_created = true; return; }
  core_loge("createthreadkey fail: %s", strerror(errnum));
  core_zero(&pthread_tls_key, sizeof(pthread_key_t));
}
static void deletethreadkey() {
  int errnum = 0;
  if (!tlskey_created) return;
  errnum = pthread_key_delete(pthread_tls_key);
  if (errnum != 0) core_loge("deletethreadkey fail: %s", strerror(errnum));
  tlskey_created = false;
}
#define CCRECORDEDSTATENUM 80 
struct ccroutine {
  lua_State* lthread;
  lua_Function lfunc;
  ccrptoto proto;
};
struct ccstate {
  _int index;
  struct ccglobal* g;
  lua_State* lstate;
  struct cclist rlist;
  pthread_t thread;
};
struct ccstatepool {
  struct ccstate s[CCRECORDEDSTATENUM];
};
struct ccglobal {
  struct ccstate* state;
  _int nstate;
  int iofd;
};
struct ccthread* ccgetmainthread(struct ccstate* s) {
  return &s->g->a[0]->thread;
}
void ccattachstate(struct ccstate* s) {
  if (s->index > 0 && s->index < CCRECORDEDSTATENUM) {
    s->g->a[s->index] = s;
  }
}
void ccdetachstate(struct ccstate* s) {
  if (s->index > 0 && s->index < CCRECORDEDSTATENUM) {
    s->g->a[s->index] = 0;
  }
}
static bool llcreatethreadkey(pthread_key_t* key) {
  /* int pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
  although the same key may be used by different threads, the values bound
  to the key are maintained on a per-thread basis and persist for the life
  of the calling thread
  an optional destructor function may be associated with each key value, at
  thread exit, if both the destructor and the value of the key are non-null,
  this destructor is called and the value of the key is set to null
  this function should called one time for one key, and it is the responsibility
  of the programmer to ensure that it is called exactly once per key before
  use of it, a typically way is to call it in the main thread */
  int n = pthread_key_create(key, 0);
  if (n != 0) { ccloge("llcreatethreadkey %s", strerror(n)); return false; }
  return true;
}
static void lldeletethreadkey(pthread_key_t key) {
  int n = pthread_key_delete(key);
  if (n != 0) { ccloge("lldeletethreadkey %s", strerror(n)); }
}
static void* llgetthreadspecific(pthread_key_t key) {
  return pthread_getspecific(key);
}
static void llsetthreadspecific(pthread_key_t key, const void* value) {
  /* different threads may bind different values to the same key, the value
  is typically a pointer to blocks of dynamically allocated memory that have
  been reserved for use by the calling thread */
  int n = pthread_setspecific(key, value);
  if (n != 0) { ccloge("llsetthreadspecific %s", strerror(n)); }
}
static void sscreatethreadkey(struct ccglobal* g) {
  if (g->keycreated) return;
  if (llgetthreadspecific(&g->key)) { g->keycreated = true; }
}
static void ssdeletethreadkey(struct ccglobal* g) {
  if (!g->keycreated) return;
  lldeletethreadkey(g->key);
  g->keycreated = false;
}
static int ssgetthreadspecific(struct ccglobal* g) {
  return (int)llgetthreadspecific(g->key);
}
static void sssetthreadspecific(struct ccglobal* g, int n) {
  llsetthreadspecific(g->key, (const void*)n); /* valid n is non-zero */
}

bool cctlskeyinit(struct cctlskey* self) {
  int n = pthread_key_create((pthread_key_t*)self, 0);
  if (n != 0) {
    ccloge("pthread_key_create %s", strerror(n));
  }
  return (n == 0);
}

void cctlskeyfree(struct cctlskey* self) {
  int n = pthread_key_delete((pthread_key_t*)self);
  if (n != 0) {
    ccloge("pthread_key_delete %s", strerror(n));
  }
}

bool cctlskeyset(struct cctlskey* self, const void* data) {
  /* different threads may bind different values to the same key, the value
  is typically a pointer to blocks of dynamically allocated memory that have
  been reserved for use by the calling thread.
  ENOMEM - insufficient memory exists to associate the value with the key.
  EINVAL - the key value is invalid. */
  int n = pthread_setspecific((pthread_key_t*)self, data);
  if (n != 0) {
    ccloge("pthread_setspecific %s", strerror(n));
  }
  return (n == 0);
}

void* cctlskeyget(struct cctlskey* self) {
  /* no errors are returned from pthread_getspecific() */
  return pthread_getspecific((pthread_key_t*)self);
}


static void llacceptconnection(void* ud, l_sockconn* conn) {
  l_service* sv = (l_service*)ud;
  ll_master_send_message(l_master_thread(), sv->svid, L_MESSAGE_CONNID, conn->sock);
}

static void ll_master_dispatch_ioevent(l_ioevent* event) {
  l_umedit svid = event->udata;
  l_thread* master = l_master_thread();
  l_service* sv = 0;
  l_umedit type = 0;

  sv = ll_find_service(svid);
  if (sv == 0) {
    l_loge_s("service not found");
    return;
  }

  if (event->fd != sv->event->fd) {
    l_loge_s("event mismatch");
    return;
  }

  ll_lock_event(sv);
  if (sv->event->flags & L_EVENT_FLAG_LISTEN) {
    if (event->masks & L_EVENT_READ) {
      type = L_MESSAGE_CONNIND;
    }
  } else if (sv->event->flags & L_EVENT_FLAG_CONNECT) {
    if (event->masks & L_EVENT_WRITE) {
      sv->event->flags &= (~((l_ushort)L_EVENT_FLAG_CONNECT));
      type = L_MESSAGE_CONNRSP;
    }
  } else {
    if (sv->event->masks == 0) {
      type = L_MESSAGE_IOEVENT;
    }
    sv->event->masks |= event->masks;
  }
  ll_unlock_event(sv);

  switch (type) {
  case L_MESSAGE_CONNIND:
    l_socket_accept(sv->event->fd, llacceptconnection, sv);
    break;
  case L_MESSAGE_CONNRSP:
    ll_master_send_message(master, sv->svid, L_MESSAGE_CONNRSP, 0);
    break;
  case L_MESSAGE_IOEVENT:
    ll_master_send_message(master, sv->svid, L_MESSAGE_IOEVENT, 0);
    break;
  }
}

void l_master_start() {
  l_squeue queue, svmsgs, frmq;
  l_message* msg = 0;
  l_service* sv = 0;
  l_thread* thread = 0;
  l_thread* master = l_master_thread();

  /* read parameters from config file and start worker threads */

  for (; ;) {
    l_ionfmgr_wait(ll_get_ionfmgr(), l_master_dispatch_event);

    /* get all received messages */
    queue = ll_get_current_msgs();
    l_squeue_push_queue(&queue, &master->txmq);

    /* prepare master's messages */
    l_squeue_init(&svmsgs);
    while ((msg = (l_message*)l_squeue_pop(&queue))) {
      if (msg->dstid == L_SERVICE_MASTERID) {
        l_squeue_push(&master->msgq, &msg->node);
      } else {
        l_squeue_push(&svmsgs, &msg->node);
      }
    }

    /* handle master's messages */
    l_squeue_init(&freeq);
    while ((msg = (l_message*)ccsqueue_pop(&master->msgq))) {
      switch (msg->type) {
      case L_MESSAGE_RUNSERVICE:
        sv = (l_service*)msg->data.ptr;
        if (sv->event) {
          l_ionfmgr_add(ll_get_ionfmgr(), sv->event);
        }
        ll_add_service_to_table(sv);
        break;
      case L_MESSAGE_ADDEVENT:
        if (msg->data.ptr) {
          l_ioevent* event = (l_ioevent*)msg->data.ptr;
          l_ionfmgr_add(ll_get_ionfmgr(), event);
        }
        break;
      case L_MESSAGE_DELEVENT:
        if (msg->data.ptr) {
          l_ioevent* event = (l_ioevent*)msg->data.ptr;
          l_ionfmgr_del(ll_get_ionfmgr(), event);
          /* need free the event after it is deleted */
          ll_free_event(event);
        }
        break;
      case L_MESSAGE_DELSERVICE:
        sv = (l_service*)msg->data.ptr;
        /* service's received msgs (if any) */
        l_squeue_pushqueue(&freeq, &sv->rxmq);
        /* detach and free event if any */
        if (sv->event) {
          l_ionfmgr_del(ll_get_ionfmgr(), sv->event);
          ll_free_event(sv->event);
          sv->event = 0;
        }
        /* remove service out from hash table and insert to free queue */
        ll_service_is_finished(sv);
        /* reset the service */
        sv->belong = 0;
        break;
      default:
        l_loge("unknow message id");
        break;
      }
      l_squeue_push(&freeq, &msg->node);
    }

    /* dispatch service's messages */
    while ((msg = (l_message*)ccsqueue_pop(&svmsgs))) {
      nauty_bool newattach = false;

      /* get message's dest service */
      sv = ll_find_service(msg->dstid);
      if (sv == 0) {
        l_logw("service not found");
        l_squeue_push(&freeq, &msg->node);
        continue;
      }

      if (sv->belong) {
        nauty_bool done = false;

        ll_lock_thread(sv->belong);
        done = (sv->outflag & L_SERVICE_COMPLETED) != 0;
        ll_unlock_thread(sv->belong);

        if (done) {
          /* free current msg and service's received msgs (if any) */
          l_squeue_push(&freeq, &msg->node);
          l_squeue_pushqueue(&freeq, &sv->rxmq);

          /* when service is done, the service is already removed out from thread's service queue.
          here delete service from the hash table and insert into service's free queue */
          ll_service_is_finished(sv);

          /* reset the service */
          sv->belong = 0;
          continue;
        }
      } else {
        /* attach a thread to handle the service */
        sv->belong = ll_get_thread_for_new_service();
        newattach = true;
      }

      /* queue the service to its belong thread */
      /*ll_lock_thread(sv->belong);
      if (newattach) {
        sv->outflag = sv->flag = 0;
        l_dqueue_push(&sv->belong->workrxq, &sv->node);
      }
      msg->extra = sv;
      l_squeue_push(&sv->rxmq, &msg->node);
      sv->belong->missionassigned = true;
      ll_unlock_thread(sv->belong); */

      thread = sv->belong;
      msg->extra = sv;
      l_thread_lock(thread);
      if (sv->
      l_squeue_push(&thread->rxmq, &msg->node);
      l_thread_unlock(thread);

      /* signal the thread to handle */
      l_condv_signal(&sv->belong->condv);
    }

    l_free_messages(master, &frmq);
  }
}

void l_worker_start() {
  l_dqueue doneq;
  l_squeue msgq;
  l_squeue freemq;
  l_service* service = 0;
  l_linknode* head = 0;
  l_linknode* elem = 0;
  l_message* msg = 0;
  l_thread* thread = l_thread_self();

  l_squeue_init(&msgq);

  for (; ;) {

    l_thread_lock(thread);
    while (l_squeue_is_empty(&thread->rxmq)) {
      l_condv_wait(&thread->condv, &thread->mutex);
    }
    l_squeue_push_queue(&msgq, &thread->rxmq);
    l_thread_unlock(thread);

    head = &(thread->workq.head);
    for (elem = head->next; elem != head; elem = elem->next) {
      service = (l_service*)elem;
      if (!ccsqueue_isempty(&service->rxmq)) {
        ll_lock_thread(thread);
        l_squeue_pushqueue(&thread->msgq, &service->rxmq);
        ll_unlock_thread(thread);
      }
    }

    if (ccsqueue_isempty(&thread->msgq)) {
      /* there are no messages received */
      continue;
    }

    l_dqueue_init(&doneq);
    l_squeue_init(&freemq);
    while ((msg = (l_message*)ccsqueue_pop(&thread->msgq))) {
      service = (l_service*)msg->extra;
      if (service->flag & L_SERVICE_COMPLETED) {
        l_squeue_push(&freemq, &msg->node);
        continue;
      }

      if (msg->type == L_MESSAGE_IOEVENT) {
        ll_lock_thread(thread);
        msg->data.u32 = service->event->masks;
        service->event->masks = 0;
        ll_unlock_thread(thread);
        if (msg->data.u32 == 0) {
          l_squeue_push(&freemq, &msg->node);
          continue;
        }
      }

#if 0
      if (service->flag & L_SERVICE_YIELDABLE) {
        if (service->co == 0) {
          service->co = ll_new_coroutine(thread, service);
        }
        if (service->co->L != thread->L) {
          l_loge("wrong lua state");
          l_squeue_push(&freemq, &msg->node);
          continue;
        }
        service->co->srvc = service;
        service->co->msg = msg;
        l_state_resume(service->co);
      } else {

      }
#endif

      /* SERVICE要么保存在hashtable中，要么保存在free队列中，
　　　　　 一个SERVICE只分配给一个线程处理，当某个SERVICE的消息到来
      时, 从消息可以获取SERVICE号，由SERVICE号从hashtable中
      取得service对象，这个service对象会保存到消息extra中，然
      后将消息插入线程的消息队列中等待线程处理 */

      service->entry(service, msg);

      /* service run completed */
      if (service->flag & L_SERVICE_COMPLETED) {
        l_dqueue_push(&doneq, l_linknode_remove(&service->node));
        if (service->co) {
          ll_free_coroutine(thread, service->co);
          service->co = 0;
        }
      }

      /* free handled message */
      l_squeue_push(&freemq, &msg->node);
    }

    while ((service = (l_service*)ccdqueue_pop(&doneq))) {
      ll_lock_thread(thread);
      service->outflag |= L_SERVICE_COMPLETED;
      ll_unlock_thread(thread);

      /* currently the finished service is removed out from thread's
      service queue, but the service still pointer to this thread.
      send SVDONE message to master to reset and free this service. */
      ll_service_send_message_sp(service, L_SERVICE_MASTERID, L_MESSAGE_DELSERVICE, service);
    }

    ll_free_msgs(&freemq);
  }
}





typedef struct {
  void* next;
} l_hashslot;

/* hash table - add/remove/search quick, need re-hash when enlarge size */
typedef struct {
  l_byte slotbits;
  l_ushort offsetofnext;
  l_umedit nslot; /* prime number size not near 2^n */
  l_umedit nelem;
  l_umedit (*getkey)(void*);
  l_hashslot* slot;
} l_hashtable;

l_extern int l_is_prime(l_umedit n);
l_extern l_umedit l_middle_prime(l_byte bits);

l_extern void l_hashtable_init(l_hashtable* self, l_byte sizebits, int offsetofnext, l_umedit (*getkey)(void*));
l_extern void l_hashtable_free(l_hashtable* self);
l_extern void l_hashtable_foreach(l_hashtable* self, void (*cb)(void*));
l_extern void l_hashtable_add(l_hashtable* self, void* elem);
l_extern void* l_hashtable_find(l_hashtable* self, l_umedit key);
l_extern void* l_hashtable_del(l_hashtable* self, l_umedit key);

/* table size is enlarged auto */
typedef struct {
  l_hashtable* cur;
  l_hashtable* old;
  l_hashtable a;
  l_hashtable b;
} l_backhash;

l_extern void l_backhash_init(l_backhash* self, l_byte initsizebits, int offsetofnext, l_umedit (*getkey)(void*));
l_extern void l_backhash_free(l_backhash* self);
l_extern void l_backhash_add(l_backhash* self, void* elem);
l_extern void* l_backhash_find(l_backhash* self, l_umedit key);
l_extern void* l_backhash_del(l_backhash* self, l_umedit key);





/**
 * hash table - add/remove/search quick, need re-hash when enlarge buffer size
 */

int l_is_prime(l_umedit n) {
  l_uinteger i = 0;
  if (n == 2) return true;
  if (n == 1 || (n % 2) == 0) return false;
  for (i = 3; i*i <= n; i += 2) {
    if ((n % i) == 0) return false;
  }
  return true;
}

/* return prime is less than 2^bits and greater then 2^(bits-1) */
l_umedit l_middle_prime(l_byte bits) {
  l_umedit i, n, mid, m = (1 << bits);
  l_umedit dn = 0, dm = 0; /* distance */
  l_umedit nprime = 0, mprime = 0;
  if (m == 0) return 0; /* max value of bits is 31 */
  if (bits < 3) return bits + 1; /* 1(2^0) 2(2^1) 4(2^2) => 1 2 3 */
  n = (1 << (bits - 1)); /* even number */
  mid = 3 * (n >> 1); /* middle value between n and m, even number */
  for (i = mid - 1; i > n; i -= 2) {
    if (l_is_prime(i)) {
      dn = i - n;
      nprime = i;
      break;
    }
  }
  for (i = mid + 1; i < m; i += 2) {
    if (l_is_prime(i)) {
      dm = m - i;
      mprime = i;
      break;
    }
  }
  return (dn > dm ? nprime : mprime);
}

void l_hashtable_init(l_hashtable* self, l_byte sizebits, int offsetofnext, l_umedit (*getkey)(void*)) {
  if (sizebits > 30) {
    l_loge_s("size too large");
    return;
  }
  if (sizebits < 3) {
    sizebits = 3;
  }
  self->slotbits = sizebits;
  self->nslot = l_middle_prime(sizebits);
  self->slot = (l_hashslot*)l_raw_calloc(sizeof(l_hashslot)*self->nslot);
  self->nelem = 0;
  self->offsetofnext = l_cast(l_ushort, offsetofnext);
  self->getkey = getkey;
}

void l_hashtable_free(l_hashtable* self) {
  if (self->slot) {
    l_raw_free(self->slot);
    self->slot = 0;
  }
  l_zero_l(self, sizeof(l_hashtable));
}

static l_umedit llgethashval(l_hashtable* self, void* elem) {
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
  self->nelem += 1;
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

void* l_hashtable_find(l_hashtable* self, l_umedit key) {
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

void* l_hashtable_del(l_hashtable* self, l_umedit key) {
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
    self->nelem -= 1;
    return elem;
  }
  prev = slot->next;
  while ((elem = llgetnextelem(self, prev)) != 0) {
    if (self->getkey(elem) == key) {
      llsetelemnext(self, prev, llgetnextelem(self, elem));
      self->nelem -= 1;
      return elem;
    }
    prev = elem;
  }
  return 0;
}

void l_backhash_init(l_backhash* self, l_byte initsizebits, int offsetofnext, l_umedit (*getkey)(void*)) {
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
  return (t->nelem > t->nslot * 2);
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
    if (oldtable.slot && oldtable.nelem) {
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

void* l_backhash_find(l_backhash* self, l_umedit key) {
  void* elem = l_hashtable_find(self->cur, key);
  if (elem == 0 && self->old->slot) {
    return l_hashtable_find(self->old, key);
  }
  return elem;
}

void* l_backhash_del(l_backhash* self, l_umedit key) {
  void* elem = l_hashtable_del(self->cur, key);
  if (elem == 0 && self->old->slot) {
    return l_hashtable_del(self->old, key);
  }
  return elem;
}



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

#define CCSOCK_ACCEPT 0x01
#define CCSOCK_CONNET 0x02

#define CCMSGTYPE_SOCKMSG 1
#define CCMSGFLAG_FREEMEM 0x0001
#define CCMSGFLAG_NEWCONN 0x0100
#define CCMSGFLAG_CONNEST 0x0200

struct ccsockmsg* ccnewsockmsg(int sockfd, umedit_int events, void* ud) {
  struct ccsockmsg* sm = (struct ccsockmsg*)ccrawalloc(sizeof(struct ccsockmsg));
  sm->head.size = sizeof(struct ccsockmsg);
  sm->head.type = CCMSGTYPE_SOCKMSG;
  sm->head.flag = CCMSGFLAG_FREEMEM;
  sm->head.data = ud;
  sm->sockfd = sockfd;
  sm->events = events;
  return sm;
}

void ccsocknewconn(struct ccepoll* self, int connfd, struct ccwork* work) {
  struct ccsockmsg* sm = ccnewsockmsg(connfd, 0, ud);
  sm->head.flag |= CCMSGFLAG_NEWCONN;
}

void ccepolldispatch(struct ccepoll* self) {
  int i = 0, n = self->n;
  struct epoll_event* ready = self->ready;
  struct ccevent* e = 0;
  for (; i < n; ++i) {
    e = (struct ccevent*)ready[i].data.ptr;
    e->revents = (umedit_int)ready[i].events;
    if ((e->revents | CCEPOLLIN) && (e->waitop & CCSOCK_ACCEPT)) {
      ccsockaccept(self, e);
    }
  }
}

struct ccionf {
  int epfd;
  int n, maxlen;
  struct epoll_event ready[CCIONF_MAX_WAIT_EVENTS];
};

struct ccionfevt {
  int fd;
  umedit_int events;
  void* ud;
};

struct ccionfmsg {
  struct ccmsghead head;
  struct ccionfevt event;
};

nauty_bool ccionfevtvoid(struct ccionfevt* self) {
  return (self->fd == -1);
}

struct ccionfmsg* ccionfmsgnew(struct ccionfevt* event) {
  struct ccionfmsg* self = (struct ccionfmsg*)ccrawalloc(sizeof(struct ccionfmsg));
  return ccionfmsgset(self, event);
}

struct ccionfmsg* ccionfmsgset(struct ccionfmsg* self, struct ccionfevt* event) {
  self->event = *event;
  return self;
}


umedit_int llionfpool_hash(struct ccionftbl* self, umedit_int fd) {
  return fd % self->size; /* size should be prime number not near 2^n */
}

umedit_int llionfpool_size(uoctect_int bits) {
  /* cchashprime(bits) return a prime number < (1 << bits) */
  return cchashprime(bits);
}

void llionfpool_addtofreelist(struct ccionftbl* self, struct ccsmplnode* node) {
  ccsmplnode_insertafter(&self->freelist, node);
  self->nfreed += 1;
}

struct ccionfmsg* llionfpool_getfromfreelist(struct ccionftbl* self, struct ccionfevt* event) {
  struct ccionfmsg* p = 0;
  if (((p = struct ccionfmsg*)ccsmplnode_removenext(&self->freelist)) == 0) {
    return 0;
  }
  if (self->nfreed > 0) {
    self->nfreed -= 1;
  } else {
    ccloge("ccionfpool freelist size invalid");
  }
  return llionfmsg_set(p, event);
}

struct ccionfpool* ccionfpool_new(uoctect_int sizenumbits) {
  struct ccionfpool* pool = 0;
  sright_int size = llionfpool_size(sizenumbits);
  pool = (struct ccionfpool*)ccrawalloc(sizeof(struct ccionfpool) + size * sizeof(struct ccionfslot));
  pool->nslot = size;
  while (size > 0) {
    ccsmplnode_init(&(pool->slot[--size].head));
  }
  ccsmplnode_init(&pool->freelist);
  ccpqueue_init(&pool->queue);
  pool->nfreed = pool->nbucket = pool->qsize = 0;
  return pool;
}

struct ccsmplnode* ccionfpool_addevent(struct ccionfpool* self, struct ccionfevt* newevent) {
  struct ccionfmsg* msg = 0;
  struct ccionfslot* slot = 0;
  struct ccsmplnode* head = 0;
  struct ccsmplnode* cur = 0;
  if (llionfevt_isempty(newevent)) {
    ccloge("addEvent invalid event");
    return 0;
  }
  slot = self->slot + ccionfpool_hash(self, (umedit_int)newevent->fd);
  for (head = &slot->head, cur = head; cur->next != head; cur = cur->next) {
    msg = (struct ccionfmsg*)cur->next;
    /* if current event is new, than it will lead all empty nodes freed */
    if (llionfevt_isempty(&msg->event)) {
      llionfpool_addtofreelist(self, ccsmplnode_removenext(cur));
      if (self->nbucket > 0) {
        self->nbucket -= 1;
      } else {
        ccloge("ccionfpool bucket size invalid");
      }
      continue;
    }
    if (msg->event.fd == newevent->fd) {
      msg->event.events |= newevent->events;
      return 0;
    }
  }
  if ((msg = llionfpool_getfromfreelist(self, newevent)) == 0) {
    msg = ccepollmsg_new(newevent);
  }
  ccsmplnode_insertafter(&slot->node, (struct ccsmplnode*)msg);
  self->nbucket += 1;
  ccpqueue_push(&self->queue, msg);
  self->qsize += 1;
  return msg;
}

int ccnodemain(struct ccstate* s) {
  struct ccglobal G;
  ccinitglobal(&G, "ccnode.conf");
  struct ccstate s[G.workers+1];
  int i = 0;
  ccinitstatepool(&G, s+1, G.workers);
  for (; i < G.workers + 1; ++i) {

  }
  ccinitstate(&s[0], G, pthread_self());
  G.iofd = llepollcreate();
  if (G.iofd == -1) return -1;
}

int main(int argc, char** argv) {
  return startmainthreadcv(ccmaster_start, argc, argv);
}
