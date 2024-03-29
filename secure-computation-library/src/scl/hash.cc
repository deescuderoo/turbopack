#include "scl/hash.h"

static inline uint64_t rotl64(uint64_t x, uint64_t y) {
  return (x << y) | (x >> ((sizeof(uint64_t) * 8) - y));
}

void scl::details::Keccakf(uint64_t state[25]) {
  uint64_t t;
  uint64_t bc[5];

  for (std::size_t round = 0; round < 24; ++round) {
    for (std::size_t i = 0; i < 5; ++i)
      bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^
              state[i + 20];

    for (std::size_t i = 0; i < 5; ++i) {
      t = bc[(i + 4) % 5] ^ rotl64(bc[(i + 1) % 5], 1);
      for (std::size_t j = 0; j < 25; j += 5) state[j + i] ^= t;
    }

    t = state[1];
    for (std::size_t i = 0; i < 24; ++i) {
      const uint64_t v = keccakf_piln[i];
      bc[0] = state[v];
      state[v] = rotl64(t, keccakf_rotc[i]);
      t = bc[0];
    }

    for (std::size_t j = 0; j < 25; j += 5) {
      for (std::size_t i = 0; i < 5; ++i) bc[i] = state[j + i];
      for (std::size_t i = 0; i < 5; ++i)
        state[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
    }

    state[0] ^= keccakf_rndc[round];
  }
}
