/**
 * @file ff.h
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
#ifndef _SCL_MATH_FF_H
#define _SCL_MATH_FF_H

#include <algorithm>
#include <string>

#include "scl/math/bases.h"
#include "scl/math/fields/def.h"
#include "scl/math/ring.h"
#include "scl/prg.h"

namespace scl {
namespace details {

/**
 * @brief Elements of the finite field \f$\mathbb{F}_p\f$ for prime \f$p\f$.
 *
 * This class defines a finite field where elements are at least as large as the
 * bitsize speficied by the template parameter. The actual size of a finite
 * field element can be queried by calling \ref scl::FF::BitSize, and will
 * depend on what finite fields are defined and how these are selected.
 *
 * For example, <code>FF<32></code> defines a finite field with where elements
 * are at least 32 bits. The actual field underlying <code>FF<32></code> is the
 * integers modulo a 61-bit prime, so <code>FF<32>::BitSize()</code> will
 * return 61.
 *
 * @tparam Bits the desired bit size of the field
 * @tparam Field the finite field. See \ref details::FiniteField
 */
template <unsigned Bits, typename Field>
class FF final : details::RingBase<FF<Bits, Field>> {
 public:
  /**
   * @brief the raw type of a finite field element.
   */
  using ValueType = typename Field::ValueType;

  /**
   * @brief Size of the field as specified in the \p Bits template parameter.
   */
  constexpr static std::size_t SpecifiedBitSize() { return Bits; };

  /**
   * @brief Size in bytes of a field element.
   */
  constexpr static std::size_t ByteSize() { return Field::kByteSize; };

  /**
   * @brief Actual bit size of an element.
   */
  constexpr static std::size_t BitSize() { return Field::kBitSize; };

  /**
   * @brief A short string representation of this field.
   */
  constexpr static const char* Name() { return Field::kName; };

  /**
   * @brief Read a field element from a buffer.
   * @param src the buffer
   * @return a field element.
   * @note This method accesses exactly \ref ByteSize() bytes of \p src.
   */
  static FF Read(const unsigned char* src) {
    FF e;
    Field::FromBytes(e.mValue, src);
    return e;
  }

  /**
   * @brief Create a random element, using a supplied PRG.
   * @param prg the PRG
   * @return a random element.
   */
  static FF Random(PRG& prg) {
    unsigned char buffer[FF<Bits, Field>::ByteSize()];
    prg.Next(buffer, FF<Bits, Field>::ByteSize());
    return FF<Bits, Field>::Read(buffer);
  };

  /**
   * @brief Create a field element from a string.
   * @param str the string
   * @param base the base of the string
   * @return a finite field element.
   */
  static FF FromString(const std::string& str, enum NumberBase base) {
    FF e;
    Field::FromString(e.mValue, str, base);
    return e;
  };

  /**
   * @brief Create a field element from a base 10 string.
   * @param str the string
   * @return a finite field element.
   */
  static FF FromString(const std::string& str) {
    return FF::FromString(str, NumberBase::DECIMAL);
  };

  /**
   * @brief Create a new element from an int.
   * @param value the value to interpret as a field element
   */
  explicit constexpr FF(int value) : mValue(Field::FromInt(value)){};

  /**
   * @brief Create a new element equal to 0 in the field.
   */
  explicit constexpr FF() : FF(0){};

  /**
   * @brief Add another field element to this.
   * @param other the other element
   * @return this set to this + \p other.
   */
  FF& operator+=(const FF& other) {
    Field::Add(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Subtract another field element to this.
   * @param other the other element
   * @return this set to this - \p other.
   */
  FF& operator-=(const FF& other) {
    Field::Subtract(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Multiply another field element to this.
   * @param other the other element
   * @return this set to this * \p other.
   */
  FF& operator*=(const FF& other) {
    Field::Multiply(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Multiplies this with the inverse of another elemenet.
   * @param other the other element
   * @return this set to this * <code>other.Inverse()</code>.
   */
  FF& operator/=(const FF& other) {
    auto copy = other.mValue;
    Field::Invert(copy);
    Field::Multiply(mValue, copy);
    return *this;
  };

  /**
   * @brief Negates this element.
   * @return this set to -this.
   */
  FF& Negate() {
    Field::Negate(mValue);
    return *this;
  };

  /**
   * @brief Computes the additive inverse of this element.
   * @return the additive inverse of this.
   */
  FF Negated() {
    auto copy = mValue;
    FF r;
    Field::Negate(copy);
    r.mValue = copy;
    return r;
  };

  /**
   * @brief Inverts this element.
   * @return this set to its inverse.
   */
  FF& Invert() {
    Field::Invert(mValue);
    return *this;
  };

  /**
   * @brief Computes the inverse of this element.
   * @return the inverse of this element.
   */
  FF Inverse() const {
    FF copy = *this;
    return copy.Invert();
  };

  /**
   * @brief Checks if this element is equal to another.
   * @param other the other element
   * @return true if this is equal to \p other.
   */
  bool Equal(const FF& other) const {
    return Field::Equal(mValue, other.mValue);
  };

  /**
   * @brief Returns a string representation of this element.
   */
  std::string ToString() const { return Field::ToString(mValue); };

  /**
   * @brief Write this element to a byte buffer.
   * @param dest the buffer. Must have space for \ref ByteSize() bytes.
   */
  void Write(unsigned char* dest) const { Field::ToBytes(dest, mValue); }

 private:
  ValueType mValue;
};
}  // namespace details

/**
 * @brief Alias for details::FF where the field template parameter is picked
 * automatically.
 */
template <unsigned Bits>
using FF = details::FF<Bits, typename details::FieldSelector<Bits>::Field>;

}  // namespace scl

#endif  // _SCL_MATH_FF_H
