#include "core/fileop.h"

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

