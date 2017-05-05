int ccnodemain(struct ccstate* s) {
  struct ccglobal G;
  ccinitglobal(&G, "ccnode.conf");
  struct ccstate s[G.workers+1];
  int i = 0;
  ccinitstatepool(&G, s+1, G.workers);
  for (; i < G.workers + 1; ++i) {

  }
  ccinitstate(&s[0], G, pthread_self());
  G.iofd = llepollcreate();
  if (G.iofd == -1) return -1;
}

