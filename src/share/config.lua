
workers = 1
-- service_table_size = 10 -- 2^10 = 1024
-- log_buffer_size = 1024*8
-- backlog = 0

http_conf = {
  ip = "";
  port = 80;
  rxlimit = 1024*8;
}

