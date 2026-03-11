#include "hash.h"

// Implements only 2 of the 3 required actions for an hashmap (insert & search)
// there is no need for deletion, the whole hashmap is reconstructed everytime
// GC happens
//
// TODO: This is not the proper way to do this, should just update the value of
// the map to point to the new raw for live strings and mark non live strings
// for deletion

// Imported straight from
// https://github.com/rurban/smhasher/blob/master/MurmurHash2.cpp

#include <stdint.h>
#include <stdlib.h>
#include "log.h"

#if defined(_MSC_VER)
#define BIG_CONSTANT(x) (x)
#else	// defined(_MSC_VER)
#define BIG_CONSTANT(x) (x##LLU)
#endif // !defined(_MSC_VER)

// Casting and type changes are added to integrate with the rest of the system
// 
// Using the 32 bit version of hash
static inline unsigned int MurmurHash2 ( const void * key, int len, uint32_t seed );

unsigned int hash(char* string, unsigned int len) {
    // TODO: should I inject a seed?
    return MurmurHash2(string, len, 0);
}

static uint32_t seed;

void randSeed() {
    seed = arc4random();
}

static inline unsigned int MurmurHash2 ( const void * key, int len, uint32_t seed ) {
  // 'm' and 'r' are mixing constants generated offline.
  // They're not really 'magic', they just happen to work well.

  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  // Initialize the hash to a 'random' value
  uint32_t h = seed ^ len;

  // Mix 4 bytes at a time into the hash
  // NOTE: This is faster than doing it 1 character at the time, bigger keys
  // NOTE: benefit from this
  const unsigned char * data = (const unsigned char *)key;
  while(len >= 4)
  {
    uint32_t k = *(uint32_t*)data;
    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  // Handle the last few bytes of the input array
  switch(len)
  {
  case 3: h ^= data[2] << 16;
  case 2: h ^= data[1] << 8;
  case 1: h ^= data[0];
          h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return (unsigned int)h;
}

// Silly way to find a prime number
static inline unsigned int isPrime(unsigned int n) {
    if (n <= 1) return 0;
    for (int i = 2; i * i <= n; i++) if (n % i == 0) return 0;
    return 1;
}

unsigned int primeProbe(unsigned int n) {
    for (unsigned int i = n; i >= 2; i--) {
        if (isPrime(i))
            return i;
    }
    return 0; // no prime found
}
