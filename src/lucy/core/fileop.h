#ifndef lucy_core_fileop_h
#define lucy_core_fileop_h
#include "core/base.h"

typedef struct {
  void* stream;
} l_file;

typedef struct {
  l_long fsize;
  l_long ctm; /* creation, utc */
  l_long atm; /* last access, utc */
  l_long mtm; /* last modify, utc */
  l_long gid;
  l_long uid;
  l_long mode;
  l_byte isfile;
  l_byte isdir;
  l_byte islink;
} l_fileattr;

typedef union {
  int unifd;
#if defined(l_plat_windows)
  HANDLE winfd;
  SOCKET wsock;
#endif
} l_filedesc;

L_INLINE int
l_filedesc_equal(l_filedesc a, l_filedesc b)
{
#if defined(l_plat_windows)
  return a.winfd == b.winfd;
#else
  return a.unifd == b.unifd;
#endif
}

L_EXTERN l_file l_file_openRead(const void* name);
L_EXTERN l_file l_file_openWrite(const void* name);
L_EXTERN l_file l_file_openAppend(const void* name);
L_EXTERN l_file l_file_openReadWrite(const void* name);
L_EXTERN l_file l_file_openReadUnbuffered(const void* name);
L_EXTERN l_file l_file_openWriteUnbuffered(const void* name);
L_EXTERN l_file l_file_openAppendUnbuffered(const void* name);
L_EXTERN l_int l_file_write(l_file* self, l_strt s);
L_EXTERN l_int l_file_writeLen(l_file* self, const void* s, l_int len);
L_EXTERN l_int l_file_read(l_file* self, void* out, l_int len);
L_EXTERN l_int l_file_put(l_file* self, l_byte ch);
L_EXTERN int l_file_get(l_file* self);
L_EXTERN int l_file_remove(const void* name);
L_EXTERN int l_file_rename(const void* from, const void* to);
L_EXTERN void l_redirect_stdout(const void* name);
L_EXTERN void l_redirect_stderr(const void* name);
L_EXTERN void l_reditect_stdin(const void* name);
L_EXTERN void l_file_close(l_file* self);
L_EXTERN void l_file_flush(l_file* self);
L_EXTERN void l_file_clearErr(l_file* self);
L_EXTERN void l_file_rewind(l_file* self);
L_EXTERN void l_file_seekFromBegin(l_file* self, long offset);
L_EXTERN void l_file_seekFromCurPos(l_file* self, long offset);
L_EXTERN int l_file_isExist(const void* name);
L_EXTERN int l_file_isExistIn(l_filedesc dirfd, const void* name);
L_EXTERN l_long l_file_getSize(const void* name);
L_EXTERN l_fileattr l_file_getAttr(const void* name);
L_EXTERN int l_file_getcwd(int (*write)(void* stream, const l_byte* dir), void* stream);
L_EXTERN int l_file_exec(const void* cmd, void (*func)(void* obj, l_strn result), void* obj);

L_EXTERN l_filedesc l_filedesc_dirfd(const void* name);
L_EXTERN void l_filedesc_close(l_filedesc* fd);
L_EXTERN l_filedesc l_filedesc_empty();
L_EXTERN int l_filedesc_isEmpty(l_filedesc fd);
L_EXTERN int l_filedesc_equal(l_filedesc a, l_filedesc b);

typedef struct {
  void* stream;
} l_dirstream;

L_EXTERN l_dirstream l_dirstream_open(const void* name);
L_EXTERN void l_dirstream_close(l_dirstream* d);
L_EXTERN const l_byte* l_dirstream_read(l_dirstream* d);

#endif /* lucy_core_fileop_h */

