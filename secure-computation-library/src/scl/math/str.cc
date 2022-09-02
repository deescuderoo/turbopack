#include "scl/math/str.h"

#include <iostream>
#include <sstream>

template <>
std::string scl::details::ToString(const std::uint64_t& v) {
  std::stringstream ss;
  ss << v;
  return ss.str();
}

template <>
std::string scl::details::ToString(const __uint128_t& v) {
  if (!v) return "0";
  auto w = v;
  std::stringstream ss;
  while (w) {
    ss << ((int)(w % 10));
    w /= 10;
  }
  auto s = ss.str();
  // s contains the number, but in reverse order
  return std::string(s.rbegin(), s.rend());
}
