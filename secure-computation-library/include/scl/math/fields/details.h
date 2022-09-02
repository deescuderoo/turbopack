/**
 * @file details.h
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
#ifndef _SCL_MATH_FIELDS_DETAILS_H
#define _SCL_MATH_FIELDS_DETAILS_H

#include <stdexcept>

namespace scl {
namespace details {

/**
 * @brief Compute a modular addition on two simple types.
 */
template <typename T>
void AddSimpleType(T& t, const T& v, const T& m) {
  t = t + v;
  if (t >= m) t = t - m;
}

/**
 * @brief Compute a modular subtraction on two simple types.
 */
template <typename T>
void SubtractSimpleType(T& t, const T& v, const T& m) {
  if (v > t)
    t = t + m - v;
  else
    t = t - v;
}

/**
 * @brief Compute the additive inverse of a simple type.
 */
template <typename T>
void NegateSimpleType(T& t, const T& m) {
  if (t) t = m - t;
}

/**
 * @brief Test equality of two simple types.
 */
template <typename T>
bool EqualSimpleType(const T& a, const T& b) {
  return a == b;
}

/**
 * @brief Compute a modular inverse of a simple type.
 */
template <typename T, typename S>
void InverseModPrimeSimpleType(T& t, const T& v, const T& m) {
#define _SCL_PARALLEL_ASSIGN(v1, v2, q) \
  do {                                  \
    const auto __temp = v2;             \
    v2 = v1 - q * __temp;               \
    v1 = __temp;                        \
  } while (0)

  if (v == 0) {
    throw std::logic_error("0 not invertible modulo prime");
  }

  S k = 0;
  S new_k = 1;
  S r = m;
  S new_r = v;

  while (new_r != 0) {
    const auto q = r / new_r;
    _SCL_PARALLEL_ASSIGN(k, new_k, q);
    _SCL_PARALLEL_ASSIGN(r, new_r, q);
  }

#undef _SCL_PARALLEL_ASSIGN

  if (k < 0) {
    k = k + m;
  }

  t = static_cast<T>(k);
}

}  // namespace details
}  // namespace scl

#endif  // _SCL_MATH_FIELDS_DETAILS_H
