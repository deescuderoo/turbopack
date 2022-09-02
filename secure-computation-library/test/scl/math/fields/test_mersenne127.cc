#include <catch2/catch.hpp>

#include "scl/math/ff.h"

using Field = scl::FF<127>;
using u128 = __uint128_t;

#define BIG_STR

TEST_CASE("Mersenne127", "[math]") {
  Field x(123);
  Field big = Field::FromString("117602807651985479001386420478188104948");

  SECTION("ToString") {
    REQUIRE(x.ToString() == "123");
    REQUIRE(big.ToString() == "117602807651985479001386420478188104948");
  }

  SECTION("Sizes") {
    REQUIRE(Field::BitSize() == 127);
    REQUIRE(Field::SpecifiedBitSize() == 127);
    REQUIRE(Field::ByteSize() == 16);

    using SmallerField = scl::FF<111>;
    REQUIRE(SmallerField::BitSize() == 127);
    REQUIRE(SmallerField::SpecifiedBitSize() == 111);
  }

  SECTION("Name") { REQUIRE(std::string(Field::Name()) == "Mersenne127"); }

  SECTION("Read/Write") {
    unsigned char buffer[Field::ByteSize()];
    big.Write(buffer);
    auto y = Field::Read(buffer);
    REQUIRE(big == y);

    x.Write(buffer);
    auto z = Field::Read(buffer);
    REQUIRE(z == x);
  }

  SECTION("FromString") {
    SECTION("Binary") {
      auto y = Field::FromString("1111011", scl::NumberBase::BINARY);
      REQUIRE(x == y);
      auto z = Field::FromString(
          "10110000111100101111010000101001101000001100101001111010010001010100"
          "00001011100000100011100011000001110000110101010110011110100",
          scl::NumberBase::BINARY);
      REQUIRE(z == big);
    }
    SECTION("Decimal") {
      auto y = Field::FromString("123", scl::NumberBase::DECIMAL);
      REQUIRE(y == x);
      auto z = Field::FromString("117602807651985479001386420478188104948",
                                 scl::NumberBase::DECIMAL);
      REQUIRE(z == big);
      auto w = Field::FromString("117602807651985479001386420478188104948");
      REQUIRE(w == z);

      REQUIRE_THROWS_MATCHES(
          Field::FromString("a", scl::NumberBase::DECIMAL),
          std::invalid_argument,
          Catch::Matchers::Message("encountered invalid decimal character"));
    }
    SECTION("Hex") {
      auto y = Field::FromString("58797a14d0653d22a05c11c60e1aacf4",
                                 scl::NumberBase::HEX);
      REQUIRE(y == big);
    }
    SECTION("Base64") {
      auto y = Field::FromString("WHl6FNBlPSKgXBHGDhqs9A==",
                                 scl::NumberBase::BASE64);
      REQUIRE(y == big);
      auto w = Field::FromString("++//", scl::NumberBase::BASE64);
      REQUIRE(w == Field(16510975));
    }
  }
}
