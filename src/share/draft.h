/** c compatibility with c++ **
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
