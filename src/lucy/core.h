#ifndef lucy_core_h
#define lucy_core_h
#include "autoconf.h"
#include "lucy/prefix.h"

#undef l_spec_inline
#undef l_spec_extern
#undef l_spec_osplat
#undef l_spec_share
#undef l_spec_thread_local
#undef l_spec_extern_thread_local
#undef l_spec_thread_local_supported

#define l_spec_inline(a) static a
#define l_spec_extern(a) extern a
#define l_spec_osplat(a) extern a

#if defined(__GNUC__)
  #define l_spec_share(a) extern a
#else
  #if defined(L_CORELIB_IMPL) || defined(L_WINDOWS_IMPL)
    #define l_spec_share(a) __declspec(dllexport) a
  #else
    #define l_spec_share(a) __declspec(dllimport) a
  #endif
#endif

#if defined(L_CLER_GCC)
  #define l_spec_thread_local(a) __thread a
  #define l_spec_extern_thread_local(a) extern __thread a
  #define l_spec_thread_local_supported
#elif defined(L_CLER_MSC)
  #define l_spec_thread_local(a) __declspec(thread) a
  #define l_spec_extern_thread_local(a) extern __declspec(thread) a
  #define l_spec_thread_local_supported
#else
  #define l_spec_thread_local(a)
#endif

#define l_const_max_rwsize (0x7fff0000) /* 2147418112 */
#define l_const_max_ubyte     l_cast(l_byte, 0xff) /* 255 */
#define l_const_max_ushort    l_cast(l_ushort, 0xffff) /* 65535 */
#define l_const_max_umedit    l_cast(l_umedit, 0xffffffff) /* 4294967295 */
#define l_const_max_ulong     l_cast(l_ulong, 0xffffffffffffffff) /* 18446744073709551615 */
#define l_const_max_sbyte     l_cast(l_sbyte, 0x7f) /* 127 */
#define l_const_max_short     l_cast(l_short, 0x7fff) /* 32767 */
#define l_const_max_medit     l_cast(l_medit, 0x7fffffff) /* 2147483647 */
#define l_const_max_long      l_cast(l_long, 0x7fffffffffffffff) /* 9223372036854775807 */
#define l_const_min_sbyte     l_cast(l_sbyte, -127-1) /* 128 0x80 */
#define l_const_min_short     l_cast(l_short, -32767-1) /* 32768 0x8000 */
#define l_const_min_medit     l_cast(l_medit, -2147483647-1) /* 2147483648 0x80000000 */
#define l_const_min_long      l_cast(l_long, -9223372036854775807-1) /* 9223372036854775808 0x8000000000000000 */

#define l_status_contread (3)
#define l_status_waitmore (2)
#define l_status_luayield (1)
#define l_status_success  (0)
#define l_status_luaerr   (-1)
#define l_status_error    (-2)
#define l_status_eread    (-3)
#define l_status_ewrite   (-4)
#define l_status_elimit   (-5)
#define l_status_ematch   (-6)
#define l_status_einval   (-7)

#if defined(L_BUILD_DEBUG)
  #define l_core_debug_here(...) { __VA_ARGS__ }
#else
  #define l_core_debug_here(...) { (void)0; }
#endif

#define l_core_zero(s, e) l_core_zero_n((s), l_core_cstr(e) - l_core_cstr(s))
#define l_core_copy(s, e, to) l_core_copy_n((s), l_core_cstr(e) - l_core_cstr(s), (to))

l_spec_extern(void)
l_core_zero_n(void* s, l_int n);

l_spec_extern(l_byte*)
l_core_copy_n(const void* s, l_int n, void* to);

typedef void* (*l_allocfunc)(void* userdata, void* buffer, l_int oldsize, l_int newsize);

l_spec_extern(void*)
l_core_alloc_func(void* userdata, void* buffer, l_int oldsize, l_int newsize);

#define l_core_malloc(func, ud, size) func((ud), 0, (size), 0)
#define l_core_calloc(func, ud, size) func((ud), 0, (size), 1)
#define l_core_ralloc(func, ud, buffer, oldsize, newsize) func((ud), (buffer), (oldsize), (newsize))
#define l_core_mfree(func, ud, buffer) func((ud), (buffer), 0, 0)

typedef struct l_linknode {
  struct l_linknode* next;
  struct l_linknode* prev;
} l_linknode;

typedef struct l_smplnode {
  struct l_smplnode* next;
} l_smplnode;

l_spec_extern(void)
l_linknode_init(l_linknode* node);

l_spec_extern(void)
l_linknode_insertAfter(l_linknode* node, l_linknode* newnode);

l_spec_extern(int)
l_linknode_isEmpty(l_linknode* node);

l_spec_extern(void)
l_smplnode_init(l_smplnode* node);

l_spec_extern(void)
l_smplnode_insertAfter(l_smplnode* node, l_smplnode* newnode);

l_spec_extern(int)
l_smplnode_isEmpty(l_smplnode* node);

l_spec_extern(l_smplnode*)
l_smplnode_removeNext(l_smplnode* node);

typedef struct l_squeue {
  l_smplnode head;
  l_smplnode* tail;
} l_squeue;

typedef struct l_dqueue {
  l_linknode head;
} l_dqueue;

l_spec_extern(void)
l_squeue_init(l_squeue* q);



#endif /* lucy_core_h */

