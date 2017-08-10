test = {
  a = 10;
  t = {b = 20; c = 30};
  d = 40;
}

workers = 1
-- log_buffer_size = 1024*8
-- service_table_size = 10 -- 2^10
-- thread_max_free_memory = 1024
-- logfile_prefix = "stdout"

http_default = {
  backlog = 0;
  ip = "";
  port = 80;
  rxlimit = 1024*8;
}

