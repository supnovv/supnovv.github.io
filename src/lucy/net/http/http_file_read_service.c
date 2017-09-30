#include "net/http/http_file_read_service.h"

/**
 * # HTTP原始文件缓存服务
 *
 * 用哈希表保存URL字符串，并将对应原始文件内容关联到字符串上。
 * URL字符串一经分配就永久存在，而文件内容则按时间顺序排序，
 * 必要时释放老的文件内容。
　*
 */

typedef struct {
  l_service head;
  l_hashtable* stbl;
  l_filedescriptor rootdir;
} l_http_file_read_service;

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

