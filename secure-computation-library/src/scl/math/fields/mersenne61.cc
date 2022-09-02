#include <cstring>
#include <iostream>
#include <sstream>

#include "scl/math/fields/def.h"
#include "scl/math/fields/details.h"
#include "scl/math/str.h"

using u64 = std::uint64_t;
using u128 = __uint128_t;

static const u64 p = 0x1FFFFFFFFFFFFFFF;
using _ = scl::details::FiniteField<scl::details::NamedField::Mersenne61>;

u64 _::FromInt(int v) { return v < 0 ? v + p : v; }

void _::Add(u64& t, const u64& v) { AddSimpleType(t, v, p); }

void _::Subtract(u64& t, const u64& v) { SubtractSimpleType(t, v, p); }

void _::Negate(u64& t) { NegateSimpleType(t, p); }

bool _::Equal(const u64& a, const u64& b) { return EqualSimpleType(a, b); }

void _::Invert(u64& t) {
  InverseModPrimeSimpleType<u64, std::int64_t>(t, t, p);
}

void _::FromString(u64& dest, const std::string& str,
                   enum scl::NumberBase base) {
  FromStringSimpleType(dest, str, base);
}

void _::Multiply(u64& t, const u64& v) {
  u128 z = (u128)t * v;
  u64 a = z >> 61;
  u64 b = (u64)z;

  a |= b >> 61;
  b &= p;

  Add(a, b);
  t = a;
}

std::string _::ToString(const u64& v) { return scl::details::ToString(v); }

void _::FromBytes(u64& dest, const unsigned char* src) {
  dest = *(const u64*)src;
  dest = dest % p;
}

void _::ToBytes(unsigned char* dest, const u64& src) {
  std::memcpy(dest, &src, sizeof(u64));
}
