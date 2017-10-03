#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#define L_LIBRARY_IMPL
#include "core/base.h"

L_EXTERN void
l_zero_n(void* start, l_int len)
{
  if (!start || len <= 0 || len > l_max_rwsize) {
    l_loge_1("size %d", ld(len));
  } else {
    memset(start, 0, (size_t)len);
  }
}

L_EXTERN l_byte*
l_copy_n(const void* from, l_int len, void* to)
{
  if (!from || len <= 0 || len > l_max_rwsize) {
    l_loge_1("size %d", ld(len));
    return 0;
  }
  if (l_cstr(to) + len <= l_cstr(from) || l_cstr(to) >= l_cstr(from) + len) {
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
  return l_cstr(to) + len;
}

L_EXTERN l_byte*
l_copy_from(l_strt s, void* to)
{
  return l_copy_n(s.start, s.end - s.start, to);
}

L_EXTERN int
l_strt_equal(l_strt a, l_strt b)
{
#if 0
  if (a.end - a.start != b.end - b.start) return false;
  while (a.start < a.end) {
    if (*a.start++ != *b.start++) return false;
  }
  return true;
#endif
  l_int len = a.end - a.start;
  if (len != b.end - b.start) return false;
  if (len < 0 || len > l_max_rwsize) {
    l_loge_1("size %d", ld(len));
    return false;
  }
  return (len == 0 || memcmp(a.start, b.start, l_cast(size_t, len)) == 0);
}

L_EXTERN int
l_strn_equal(l_strn a, l_strn b)
{
  if (a.len != b.len) return false;
  if (a.len < 0 || a.len > l_max_rwsize) {
    l_loge_1("size %d", ld(a.len));
    return false;
  }
  return (a.len == 0 || memcmp(a.start, b.start, l_cast(size_t, a.len)) == 0);
}

L_EXTERN const l_byte*
l_strt_contain(l_strt s, int ch)
{
#if 0
  while (s.start < s.end) {
    if (*s.start++ == ch) return s.start - 1;
  }
  return 0;
#endif
  l_int len = s.end - s.start;
  if (!s.start || len <= 0 || len > l_max_rwsize) {
    l_loge_1("size %d", ld(len));
    return 0;
  }
  return memchr(s.start, ch, l_cast(size_t, len));
}

L_EXTERN const l_byte*
l_strn_contain(l_strn s, int ch)
{
  if (!s.start || s.len <= 0 || s.len > l_max_rwsize) {
    l_loge_1("size %d", ld(s.len));
    return 0;
  }
  return memchr(s.start, ch, l_cast(size_t, s.len));
}

static void*
l_out_of_memory(l_int size, int init)
{
  l_process_exit();
  (void)size;
  (void)init;
  return 0;
}

static l_int
l_check_alloc_size(l_int size)
{
  if (size <= 0 || size > L_MAX_RWSIZE) return 0;
  return (((size - 1) >> 3) + 1) << 3; /* times of eight */
}

l_assert(l_check_alloc_size(-1) == 0);
l_assert(l_check_alloc_size(0) == 0);
l_assert(l_check_alloc_size(1) == 8);
l_assert(l_check_alloc_size(2) == 8);
l_assert(l_check_alloc_size(7) == 8);
l_assert(l_check_alloc_size(8) == 8);
l_assert(l_check_alloc_size(9) == 16);
l_assert(l_check_alloc_size(L_MAX_RWSIZE-1) == L_MAX_RWSIZE);
l_assert(l_check_alloc_size(L_MAX_RWSIZE) == L_MAX_RWSIZE);
l_assert(l_check_alloc_size(L_MAX_RWSIZE+1) == 0);

static void*
l_raw_alloc_m(l_int size)
{
  void* p = 0;
  l_int n = l_check_alloc_size(size);
  if (!n) {
    l_loge_1("large %d", ld(size));
  } else {
    if (!(p = malloc(l_cast(size_t, n)))) {
      p = l_out_of_memory(n, 0);
    }
  }
  return p; /* the memory is not initialized */
}

static void*
l_raw_alloc_c(l_int size) {
  void* p = 0;
  l_int n = l_check_alloc_size(size);
  if (!n) { l_loge_1("large %d", ld(size)); return 0; }
  /* void* calloc(size_t num, size_t size); */
  p = calloc(l_cast(size_t, n) >> 3, 8);
  if (p) return p;
  return l_out_of_memory(n, 1);
}

static void*
l_raw_alloc_r(void* p, l_int old, l_int newsz) {
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

#define l_malloc(allocfunc, ud, size) allocfunc((ud), 0, (size), 0)
#define l_calloc(allocfunc, ud, size) allocfunc((ud), 0, (size), 1)
#define l_ralloc(allocfunc, ud, buffer, oldsize, newsize) allocfunc((ud), (buffer), (oldsize), (newsize))
#define l_mfree(allocfunc, ud, buffer) allocfunc((ud), (buffer), 0, 0)

L_EXTERN void*
l_raw_alloc_func(void* userdata, void* buffer, l_int oldsize, l_int newsize)
{
  (void)userdata;
  if (!buffer) {
    return newsize ? l_raw_alloc_c(oldsize) : l_raw_alloc_m(oldsize);
  }
  if (newsize) {
    return l_raw_alloc_r(buffer, oldsize, newsize);
  }
  l_raw_alloc_f(buffer);
  return 0;
}


#endif

L_EXTERN void
l_core_base_test()
{
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
  pstr->start = l_cstr(buffer);
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
  l_assert(sizeof(l_eightbyte) == 8);
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

