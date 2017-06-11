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

