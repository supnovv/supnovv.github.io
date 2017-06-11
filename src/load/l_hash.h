#ifndef CLIB_HASHVAL32_H_
#define CLIB_HASHVAL32_H_

umedit ccdhash(umedit key) {
  return key % CC_HASH_PRIME_SLOTS; /* choise a prime number not near 2^n */
}

umedit ccmhash(umedit key) {
  /* slot = 2 ^ p   A = (sqrt(5) - 1)/2 = s / (2 ^ 32)   s = 2654435769.4972302964775847707926
  hash = slot * key * A = (2 ^ p) * key * s / (2 ^ 32) = key * s / (2 ^ (32 - p)) */
  return ((key * 2654435769) >> (32 - CC_HASH_NSLOT_BITS)); /* high bits are more important than lower bits */
}

umedit ccshash(const byte* s, umedit len) {
  umedit i = 0, n = 0, t = 0;
  for (; i < len; ++i) {
    n = (n << 4) + s[i];
    if ((t = (n & 0xF0000000))) {
      n = n ^ (t >> 24);
      n = n ^ t;
    }
  }
  return ccdhash(n);
}

#endif /* CLIB_HASHVAL32_H_ */
