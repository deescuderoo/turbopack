#include <catch2/catch.hpp>

#include "scl/math.h"
#include "scl/prg.h"
#include "scl/ss/additive.h"

TEST_CASE("AdditiveSS", "[ss]") {
  using FF = scl::FF<61>;
  scl::PRG prg;

  auto secret = FF(12345);

  auto shares = scl::CreateAdditiveShares(secret, 10, prg);
  REQUIRE(shares.Size() == 10);
  REQUIRE(scl::ReconstructAdditive(shares) == secret);

  auto x = FF(55555);
  auto shr_x = scl::CreateAdditiveShares(x, 10, prg);
  auto sum = shares.Add(shr_x);

  REQUIRE(scl::ReconstructAdditive(sum) == secret + x);

  REQUIRE_THROWS_MATCHES(
      scl::CreateAdditiveShares(secret, 0, prg), std::invalid_argument,
      Catch::Matchers::Message("cannot create shares for 0 people"));
}
