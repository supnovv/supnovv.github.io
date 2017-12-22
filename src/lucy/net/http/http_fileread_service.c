#include "net/http/http_fileread_service.h"

/**
 * # HTTP原始文件缓存服务
 *
 * 用哈希表保存URL字符串，并将对应原始文件内容关联到字符串上。
 * URL字符串一经分配就永久存在，而文件内容则按时间顺序排序，
 * 必要时释放老的文件内容。
　*
 * 接收URL字符串，计算哈希值，匹配哈希表
 */

typedef struct {
  l_service head;
  l_hashtable* stbl;
  l_filedescriptor rootdir;
  l_umedit seed;
} l_http_fileread_service;

/*
   request folder - [url, filename)
   file name - [filename, suffix)
   file suffix - [suffix, query)
   query - [query, urlend)
*/

typedef struct {
  const l_byte* path;
  const l_byte* filename;
  const l_byte* suffix;
  const l_byte* query;
  const l_byte* urlend;
  l_service* srcid;
} l_http_url_parts;

static l_strn ll_get_file_basename(l_http_url_parts* url);
static l_strn ll_get_file_suffix(l_http_url_parts* url);

#define L_STRUCT_ARRT(name, size) typedef struct { l_arrt_base base; l_byte abuf[size]; } l_arrt_##name;
#define l_define_arrt(name, x) l_arrt_##name x; x.base.alen = 0; x.base.amax = sizeof(l_arrt_##name) - sizeof(l_arrt_base);

typedef struct {
  l_umedit alen;
  l_umedit amax;
} l_arrt_base;

L_INLINE l_byte*
l_arrt_get(l_arrt_base* self)
{
  return (l_byte*)(self + 1);
}

L_STRUCT_ARRT(path, FILENAME_MAX);

static int
l_arrt_append(l_arrt_base* a, l_strt s)
{
  if (s.start >= s.end) return true;
  if (a->alen >= a->amax) return false;
  a->alen += s.len;
  buf = l_arrt_get(a);
  while (s.len--) {
    buf[a->alen++] = 
  }
  return true;
}

static void
ll_handle_url_request(l_http_fileread_service* srvc, l_http_url_parts* url)
{
  l_strn folder = ll_get_url_folder(url);
  l_strn basename = ll_get_file_basename(url);
  l_strn suffix = ll_get_file_suffix(url);

  if (basename.len && suffix.len) {
    l_strt path = l_strt_from(url->path, url->query);
    ll_handle_url_request_impl(srvc, l_strn_from(url->path, url->query), ll_get_url_query(url));
  } else {
    l_define_arrt(path, path_buf);
    if (!basename.len) {
      basename = ll_http_get_default_basename(folder);
    }
    if (!l_arrt_append(&path_buf.base, basename)) {
      l_loge_s("path append basename failed");
      /* TODO: response error message */
      return;
    }
    if (!suffix.len) {
      suffix = ll_http_get_default_suffix(folder, basename);
    }
    if (!l_arrt_append(&path_buf.base, suffix)) {
      l_loge_s("path append suffix failed");
      /* TODO: response error message */
      return;
    }
    ll_handle_url_request_impl(srvc, l_arrt_strn(&path_buf.base), ll_get_url_query(url));
  }
}

static void
ll_handle_url_request_impl(l_http_fileread_service* srvc, l_strn path, l_strn query)
{
  l_umedit hash = l_string_hash(path, srvc->seed);

}

static int
http_fileread_service_proc(l_service* srvc, l_message* msg)
{

}

l_static(int)
l_http_file_read_service_proc(l_service* srvc, l_message* msg) {
  l_http_file_read_service* frsv = (l_http_file_read_service*)srvc;
  l_message_ptrn* urlmsg = (l_message_ptrn*)msg;

}

l_static(void)
l_http_file_read_service_free_table_element(l_smplnode* elem) {
  l_shortstr* s = (l_shortstr*)elem;
}

l_static(void)
l_http_file_read_service_free(l_service* srvc) {
  l_http_file_read_service* frsv = (l_http_file_read_service*)srvc;
  l_hashtable_free(&frsv->stbl, l_http_file_read_service_free_table_element);
  l_close_fd(&frsv->rootdir);
}

l_extern(void)
l_http_file_read_service_start(l_byte hashtable_size_bits, const void* root_dir_path, void (*url_handle_rule)()) {
  l_http_file_read_service* frsv = 0;

  frsv = (l_http_file_read_service*)l_create_service(sizeof(l_http_file_read_service), l_http_file_read_service_proc, l_http_file_read_service_free);
  frsv->stbl = l_hashtable_create(hashtable_size_bits);
  frsv->rootdir = l_get_dirfd(root_dir_path);

  l_start_service(&frsv->head);
  return 0;
}

