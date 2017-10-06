#include "core/fileop.h"

static l_file
llopenfile(const void* name, const char* mode)
{
  l_file fs = {0};
  if (!name || !mode) {
    l_loge_s("invalid argument");
  } else {
    if (!(fs.stream = fopen((const char*)name, mode))) {
      l_loge_1("fopen %s", lserror(errno));
    }
  }
  return fs;
}

static void
llreopenfile(FILE* file, const void* name, const char* mode)
{
  if (!name || !mode) {
    l_loge_s("invalid argument");
    return;
  }
  if (freopen((const char*)name, mode, file) == 0) {
    l_loge_1("freopen %s", lserror(errno));
  }
}

L_EXTERN l_file
l_file_openRead(const void* name)
{
  return llopenfile(name, "rb");
}

L_EXTERN l_file
l_file_openWrite(const void* name)
{
  return llopenfile(name, "wb");
}

L_EXTERN l_file
l_file_openAppend(const void* name)
{
  return llopenfile(name, "ab");
}

L_EXTERN l_file
l_file_openReadWrite(const void* name)
{
  return llopenfile(name, "rb+");
}

L_EXTERN l_file
l_file_openReadUnbuffered(const void* name)
{
  l_file fs = llopenfile(name, "rb");
  if (!fs.stream) return fs;
  setbuf((FILE*)fs.stream, 0);
  return fs;
}

L_EXTERN l_file
l_file_openWriteUnbuffered(const void* name)
{
  l_filestream fs = llopenfile(name, "wb");
  if (!fs.stream) return fs;
  setbuf((FILE*)fs.stream, 0);
  return fs;
}

L_EXTERN l_file
l_file_openAppendUnbuffered(const void* name)
{
  l_file fs = llopenfile(name, "ab");
  if (!fs.stream) return fs;
  setbuf((FILE*)fs.stream, 0);
  return fs;
}

L_EXTERN int
l_file_remove(const void* name)
{
  if (!name) {
    l_loge_s("empty name");
    return L_STATUS_EINVAL;
  }
  if (remove((const char*)name) != 0) {
    l_loge_1("remove %s", lserror(errno));
    return L_STATUS_ERROR;
  }
  return 0;
}

L_EXTERN int
l_file_rename(const void* from, const void* to)
{
  /* int rename(const char* oldname, const char* newname);
  Changes the name of the file or directory specified by
  oldname to newname. This is an operation performed directly
  on a file; No streams are involved in the operation. If
  oldname and newname specify different paths and this is
  supported by the system, the file is moved to the new
  location. If newname names an existing file, the function
  may either fail or override the existing file, depending on
  the specific system and library implementation. */
  if (!from || !to) {
    l_loge_s("empty name");
    return L_STATUS_EINVAL;
  }
  if (rename((const char*)from, (const char*)to) != 0) {
    l_loge_1("rename %s", lserror(errno));
    return L_STATUS_ERROR;
  }
  return 0;
}

L_EXTERN void
l_redirect_stdout(const void* name)
{
  llreopenfile(stdout, name, "wb");
}

L_EXTERN void
l_redirect_stderr(const void* name)
{
  llreopenfile(stderr, name, "wb");
}

L_EXTERN void
l_reditect_stdin(const void* name)
{
  llreopenfile(stdin, name, "rb");
}

L_EXTERN void
l_file_close(l_file* self)
{
  if (!self->stream) return;
  if (fclose((FILE*)self->stream) != 0) {
    l_loge_1("fclose %s", lserror(errno));
  }
  self->stream = 0;
}

L_EXTERN void
l_file_flush(l_file* self)
{
  if (fflush((FILE*)self->stream) != 0) {
    l_loge_1("fflush %s", lserror(errno));
  }
}

L_EXTERN void
l_file_rewind(l_file* self)
{
  rewind((FILE*)self->stream);
}

L_EXTERN void
l_file_seekFromBegin(l_file* self, long offset)
{
  if (fseek((FILE*)self->stream, offset, SEEK_SET) != 0) {
    l_loge_1("fseek SET %s", lserror(errno));
  }
}

L_EXTERN void
l_file_seekFromCurPos(l_file* self, long offset)
{
  if (fseek((FILE*)self->stream, offset, SEEK_CUR) != 0) {
    l_loge_1("fseek CUR %s", lserror(errno));
  }
}

L_EXTERN void
l_file_clearerr(l_file* self)
{
  clearerr((FILE*)self->stream);
}

L_EXTERN l_int
l_file_write(l_file* self, l_strt s)
{
  return l_write_file(self, s.start, s.end - s.start);
}

L_EXTERN l_int
l_file_writeLen(l_file* self, const void* s, l_int len)
{
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

L_EXTERN l_int
l_file_read(l_file* self, void* out, l_int len)
{
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

