/**
 * @file hash.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#ifndef _SCL_HASH_H
#define _SCL_HASH_H

#include <array>
#include <cstdint>
#include <vector>

namespace scl {
namespace details {

/**
 * @brief A hash function.
 *
 * Hash defines an Initialize-Update-Finalize style interface for a hash
 * function (concretely, SHA-3). The overloads to update mimic those in PRG.
 *
 * The implementation is based on SHA-3 reference implementation which can be
 * found at https://github.com/brainhub/SHA3IUF.
 *
 * @tparam tag a Tag specifying the type of SHA-3 use.
 */
template <std::size_t B>
class Hash {
  static_assert(B == 256 || B == 384 || B == 512,
                "B must be one of 256, 384 or 512");

 public:
  /**
   * @brief The type of the final digest. Will be an STL array of a some size.
   */
  using DigestType = std::array<unsigned char, B / 8>;

  /**
   * @brief Initialize the hash function.
   */
  Hash(){};

  /**
   * @brief Update the hash function with a set of bytes.
   *
   * @param bytes a pointer to a number of bytes.
   * @param nbytes the number of bytes.
   * @return the updated Hash object.
   */
  Hash &Update(const unsigned char *bytes, std::size_t nbytes);

  /**
   * @brief Update the hash function with the content from a byte vector.
   *
   * @param bytes a vector of bytes.
   * @return the updated Hash object.
   */
  Hash &Update(const std::vector<unsigned char> &bytes) {
    return Update(bytes.data(), bytes.size());
  };

  /**
   * @brief Finalize and return the digest.
   */
  DigestType Finalize();

 private:
  static const std::size_t kStateSize = 25;
  static const std::size_t kCapacity = 2 * B / (8 * sizeof(uint64_t));
  static const std::size_t kCuttoff = kStateSize - (kCapacity & (~0x80000000));

  uint64_t mState[kStateSize] = {0};
  unsigned char mStateBytes[kStateSize * 8] = {0};
  uint64_t mSaved = 0;
  unsigned int mByteIndex = 0;
  unsigned int mWordIndex = 0;
};

static const uint64_t keccakf_rndc[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL};

static const unsigned int keccakf_rotc[24] = {1,  3,  6,  10, 15, 21, 28, 36,
                                              45, 55, 2,  14, 27, 41, 56, 8,
                                              25, 43, 62, 18, 39, 61, 20, 44};

static const unsigned int keccakf_piln[24] = {10, 7,  11, 17, 18, 3,  5,  16,
                                              8,  21, 24, 4,  15, 23, 19, 13,
                                              12, 2,  20, 14, 22, 9,  6,  1};

/**
 * @brief Keccak function.
 * @param state the current state
 */
void Keccakf(uint64_t state[25]);

template <std::size_t B>
Hash<B> &Hash<B>::Update(const unsigned char *bytes, std::size_t nbytes) {
  unsigned int old_tail = (8 - mByteIndex) & 7;
  const unsigned char *p = bytes;

  if (nbytes < old_tail) {
    while (nbytes--) mSaved |= (uint64_t)(*(p++)) << ((mByteIndex++) * 8);
    return *this;
  }

  if (old_tail) {
    nbytes -= old_tail;
    while (old_tail--) mSaved |= (uint64_t)(*(p++)) << ((mByteIndex++) * 8);

    mState[mWordIndex] ^= mSaved;
    mByteIndex = 0;
    mSaved = 0;

    if (++mWordIndex == kCuttoff) {
      Keccakf(mState);
      mWordIndex = 0;
    }
  }

  std::size_t words = nbytes / sizeof(uint64_t);
  unsigned int tail = nbytes - words * sizeof(uint64_t);

  for (std::size_t i = 0; i < words; ++i) {
    const uint64_t t =
        (uint64_t)(p[0]) | ((uint64_t)(p[1]) << 8 * 1) |
        ((uint64_t)(p[1]) << 8 * 2) | ((uint64_t)(p[1]) << 8 * 3) |
        ((uint64_t)(p[1]) << 8 * 4) | ((uint64_t)(p[1]) << 8 * 5) |
        ((uint64_t)(p[1]) << 8 * 6) | ((uint64_t)(p[1]) << 8 * 7);

    mState[mWordIndex] ^= t;

    if (++mWordIndex == kCuttoff) {
      Keccakf(mState);
      mWordIndex = 0;
    }
    p += sizeof(uint64_t);
  }

  while (tail--) mSaved |= (uint64_t)(*(p++)) << ((mByteIndex++) * 8);

  return *this;
}

template <std::size_t B>
auto Hash<B>::Finalize() -> DigestType {
  uint64_t t = (uint64_t)(((uint64_t)(0x02 | (1 << 2))) << ((mByteIndex)*8));
  mState[mWordIndex] ^= mSaved ^ t;
  mState[kCuttoff - 1] ^= 0x8000000000000000ULL;
  Keccakf(mState);

  for (std::size_t i = 0; i < kStateSize; ++i) {
    const unsigned int t1 = (uint32_t)mState[i];
    const unsigned int t2 = (uint32_t)((mState[i] >> 16) >> 16);
    mStateBytes[i * 8 + 0] = (unsigned char)t1;
    mStateBytes[i * 8 + 1] = (unsigned char)(t1 >> 8);
    mStateBytes[i * 8 + 2] = (unsigned char)(t1 >> 16);
    mStateBytes[i * 8 + 3] = (unsigned char)(t1 >> 24);
    mStateBytes[i * 8 + 4] = (unsigned char)t2;
    mStateBytes[i * 8 + 5] = (unsigned char)(t2 >> 8);
    mStateBytes[i * 8 + 6] = (unsigned char)(t2 >> 16);
    mStateBytes[i * 8 + 7] = (unsigned char)(t2 >> 24);
  }

  // truncate
  DigestType digest = {0};
  for (std::size_t i = 0; i < digest.size(); ++i) digest[i] = mStateBytes[i];

  return digest;
}

}  // namespace details

/**
 * @details A hash function with 256 bit output.
 */
using Hash256 = details::Hash<256>;

/**
 * @details A hash function with 384 bit output.
 */
using Hash384 = details::Hash<384>;

/**
 * @details A hash function with 512 bit output.
 */
using Hash512 = details::Hash<512>;

}  // namespace scl

#endif  // _SCL_HASH_H
