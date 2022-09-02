#ifndef _SCUTIL_MATH_GF7_H
#define _SCUTIL_MATH_GF7_H

#include <iostream>
#include <sstream>

#include "scl/math/bases.h"

namespace scl {
namespace details {

struct GF7 {
  using ValueType = unsigned char;
  constexpr static const char* kName = "GF(7)";
  constexpr static const std::size_t kByteSize = 1;
  constexpr static const std::size_t kBitSize = 8;
  static ValueType FromInt(int x) {
    auto r = x % 7;
    auto v = r < 0 ? 7 + r : r;
    return v;
  };
  static void Add(ValueType& t, const ValueType& b) { t = (t + b) % 7; };
  static void Subtract(ValueType& t, const ValueType& v) {
    if (t < v)
      t = 7 + t - v;
    else
      t = t - v;
  };
  static void Multiply(ValueType& t, const ValueType& v) { t = (t * v) % 7; };
  static void Negate(ValueType& t) { t = (7 - t) % 7; }
  static void Invert(ValueType& t) {
    ValueType inv;
    switch (t) {
      case 1:
      case 6:
        inv = t;
        break;
      case 2:
        inv = 4;
        break;
      case 3:
        inv = 5;
        break;
      case 4:
        inv = 2;
        break;
      case 5:
        inv = 3;
        break;
      default:
        throw std::logic_error("0 not invertible modulo prime");
    }

    t = inv;
  };
  static bool Equal(const ValueType& a, const ValueType& b) { return a == b; }
  static void ToBytes(unsigned char* dest, const ValueType& src) {
    *dest = src;
  }
  static void FromBytes(ValueType& dest, const unsigned char* src) {
    dest = *src;
    dest = dest % 7;
  }
  static void FromString(ValueType& dest, const std::string& str,
                         enum NumberBase base) {
    (void)str;
    (void)base;
    dest = 0;
  }
  static std::string ToString(const ValueType& v) {
    std::stringstream ss;
    ss << (int)v;
    return ss.str();
  };
};

}  // namespace details
}  // namespace scl

#endif /* _SCUTIL_MATH_GF7_H */
