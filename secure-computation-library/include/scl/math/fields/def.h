/**
 * @file def.h
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
#ifndef _SCL_MATH_FIELDS_DEF_H
#define _SCL_MATH_FIELDS_DEF_H

#include <cstdint>
#include <string>
#include <type_traits>

#include "scl/math/bases.h"

namespace scl {
namespace details {

/**
 * @brief Known implementations of various finite fields.
 */
enum class NamedField {
  //! @brief Indicates that the user tried to create a field with bit-level 0.
  InvalidField,

  //! @brief Finite field \f$\mathbb{Z}_p\f$ with \f$p=2^{61}-1\f$.
  Mersenne61,

  //! @brief Finite field \f$\mathbb{Z}_p\f$ with \f$p=2^{127}-1\f$.
  Mersenne127
};

/**
 * @brief A finite field definition.
 * @tparam Bits the number of bits per element of the field
 */
template <enum NamedField>
struct FiniteField {};

/**
 * @brief Provide a new finite field definition.
 *
 * This macro will define a type with a provided name, as well as an appropriate
 * template specialization for FieldSelector.
 *
 * The type defined will include prototypes for functions which define
 * operations between field elements of the type \p internal_type. These
 * prototypes have to be implemented somewhere.
 *
 * See for example the source files for the already defined
 * <code>Mersenne61</code> and <code>Mersenne127</code> types.
 *
 * @param name the type name of the finite field
 * @param name_as_string a string representation of this finite field
 * @param bits the bitsize of the field. Must be unique
 * @param internal_type the type used for internally representing field elements
 */
#define DEFINE_FINITE_FIELD(name, name_as_string, bits, internal_type) \
  template <>                                                          \
  struct FiniteField<name> {                                           \
    using ValueType = internal_type;                                   \
    constexpr static const char* kName = name_as_string;               \
    constexpr static const std::size_t kByteSize = sizeof(ValueType);  \
    constexpr static const std::size_t kBitSize = bits;                \
    static ValueType FromInt(int);                                     \
    static void Add(ValueType& t, const ValueType& v);                 \
    static void Subtract(ValueType& t, const ValueType& v);            \
    static void Multiply(ValueType& t, const ValueType& v);            \
    static void Negate(ValueType& t);                                  \
    static void Invert(ValueType& t);                                  \
    static bool Equal(const ValueType& a, const ValueType& b);         \
    static void ToBytes(unsigned char* dest, const ValueType& src);    \
    static void FromBytes(ValueType& dest, const unsigned char* src);  \
    static void FromString(ValueType& dest, const std::string& str,    \
                           enum NumberBase base);                      \
    static std::string ToString(const ValueType& v);                   \
  }

/**
 * @brief Mersenne61.
 */
DEFINE_FINITE_FIELD(NamedField::Mersenne61, "Mersenne61", 61, std::uint64_t);

/**
 * @brief Mersenne127.
 */
DEFINE_FINITE_FIELD(NamedField::Mersenne127, "Mersenne127", 127, __uint128_t);

/**
 * @brief Select a suitable Finite Field based on a provided bitlevel
 */
template <unsigned Bits>
struct FieldSelector {
  //! The finite field type suitable for \p Bits bits of computation.
  using Field =
      FiniteField<(Bits >= 1 && Bits <= 61)     ? NamedField::Mersenne61
                  : (Bits >= 62 && Bits <= 127) ? NamedField::Mersenne127
                                                : NamedField::InvalidField>;
};

}  // namespace details
}  // namespace scl

#endif  // _SCL_MATH_FIELDS_DEF_H
