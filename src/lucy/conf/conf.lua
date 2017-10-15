test = {
  a = 10;
  t = {b = 20; c = 30};
  d = 40;
}

workers = 0
-- log_buffer_size = 1024*8
-- service_table_size = 10 -- 2^10
-- thread_max_free_memory = 1024
-- logfile_prefix = "stdout"

http_default = {
  ip = "127.0.0.1";
  port = 80;
  backlog = 0;
  tx_init_size = 0;
  rx_init_size = 0;
  rx_limit = 1024*8;
}

