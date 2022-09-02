#include <catch2/catch.hpp>

#include "scl/math/ff.h"

using Field = scl::FF<61>;

TEST_CASE("Mersenne61", "[math]") {
  Field x(123);
  Field big(4284958);

  SECTION("ToString") {
    REQUIRE(x.ToString() == "123");
    REQUIRE(big.ToString() == "4284958");
  }

  SECTION("Sizes") {
    REQUIRE(Field::BitSize() == 61);
    REQUIRE(Field::SpecifiedBitSize() == 61);
    REQUIRE(Field::ByteSize() == 8);

    using SmallerField = scl::FF<58>;
    REQUIRE(SmallerField::BitSize() == 61);
    REQUIRE(SmallerField::SpecifiedBitSize() == 58);
  }

  SECTION("Name") { REQUIRE(std::string(Field::Name()) == "Mersenne61"); }

  SECTION("Read/write") {
    unsigned char buffer[Field::ByteSize()];
    x.Write(buffer);
    auto y = Field::Read(buffer);
    REQUIRE(x == y);
  }

  SECTION("FromString") {
    SECTION("Binary") {
      auto y = Field::FromString("1111011", scl::NumberBase::BINARY);
      REQUIRE(y == x);
      auto z =
          Field::FromString("10000010110001000011110", scl::NumberBase::BINARY);
      REQUIRE(z == big);

      REQUIRE_THROWS_MATCHES(
          Field::FromString("2", scl::NumberBase::BINARY),
          std::invalid_argument,
          Catch::Matchers::Message("encountered invalid binary character"));
    }
    SECTION("Decimal") {
      auto y = Field::FromString("123", scl::NumberBase::DECIMAL);
      REQUIRE(y == x);
      auto z = Field::FromString("4284958", scl::NumberBase::DECIMAL);
      REQUIRE(z == big);
      auto w = Field::FromString("4284958");
      REQUIRE(w == z);

      REQUIRE_THROWS_MATCHES(
          Field::FromString("a", scl::NumberBase::DECIMAL),
          std::invalid_argument,
          Catch::Matchers::Message("encountered invalid decimal character"));
    }
    SECTION("Hex") {
      REQUIRE_THROWS_MATCHES(Field::FromString("012", scl::NumberBase::HEX),
                             std::invalid_argument,
                             Catch::Matchers::Message("odd-length hex string"));
      REQUIRE_THROWS_MATCHES(
          Field::FromString("1g", scl::NumberBase::HEX), std::invalid_argument,
          Catch::Matchers::Message("encountered invalid hex character"));
      auto y = Field::FromString("7b", scl::NumberBase::HEX);
      REQUIRE(x == y);
      auto z = Field::FromString("41621E", scl::NumberBase::HEX);
      REQUIRE(z == big);
    }
    SECTION("Base64") {
      // invalid length
      REQUIRE_THROWS_MATCHES(
          Field::FromString("a", scl::NumberBase::BASE64),
          std::invalid_argument,
          Catch::Matchers::Message("invalid length base64 string"));

      // invalid characters
      REQUIRE_THROWS_MATCHES(
          Field::FromString("w==a", scl::NumberBase::BASE64),
          std::invalid_argument,
          Catch::Matchers::Message("encountered invalid base64 character"));
      REQUIRE_THROWS_MATCHES(
          Field::FromString("wa)a", scl::NumberBase::BASE64),
          std::invalid_argument,
          Catch::Matchers::Message("encountered invalid base64 character"));

      // invalid padding
      REQUIRE_THROWS_MATCHES(
          Field::FromString("====", scl::NumberBase::BASE64),
          std::invalid_argument,
          Catch::Matchers::Message("invalid base64 padding"));
      REQUIRE_THROWS_MATCHES(
          Field::FromString("w===", scl::NumberBase::BASE64),
          std::invalid_argument,
          Catch::Matchers::Message("invalid base64 padding"));
      REQUIRE_THROWS_MATCHES(
          Field::FromString("w=a=", scl::NumberBase::BASE64),
          std::invalid_argument,
          Catch::Matchers::Message("invalid base64 padding"));

      auto y = Field::FromString("ew==", scl::NumberBase::BASE64);
      REQUIRE(y == x);

      auto z = Field::FromString("QWIe", scl::NumberBase::BASE64);
      REQUIRE(z == big);

      auto w = Field::FromString("K5M=", scl::NumberBase::BASE64);
      REQUIRE(w == Field(11155));
    }
  }
}
