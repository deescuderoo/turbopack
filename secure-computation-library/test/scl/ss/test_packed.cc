#include <catch2/catch.hpp>

#include "../gf7.h"
#include "scl/math/ff.h"
#include "scl/math/vec.h"
#include "scl/prg.h"
#include "scl/ss/shamir.h"
#include "scl/ss/packed.h"

TEST_CASE("Packed-SS", "[pss]") {
  using FF = scl::FF<61>;
  using Vec = scl::Vec<FF>;
  
  scl::PRG prg;

  // k=2, d=3
  std::size_t degree(10);
  std::size_t n_shares = degree+1;

  SECTION("FromSecretAndDegree") {
    auto secret = FF(123);
    auto poly = scl::details::EvPolyFromSecretAndDegree(secret, degree, prg);
    auto shares = scl::details::SharesFromEvPoly(poly, n_shares);

    REQUIRE(shares.Size() == n_shares);

    auto reconstructed = scl::details::SecretFromShares(shares);
    REQUIRE(reconstructed == secret);
  }

  SECTION("EvPolyFromSecretAndPointAndDegree") {
    auto secret = FF(123);
    auto x_point = FF(-42);
    auto poly = scl::details::EvPolyFromSecretAndPointAndDegree(secret, x_point, degree, prg);
    auto shares = scl::details::SharesFromEvPoly(poly, n_shares);

    REQUIRE(shares.Size() == n_shares);

    auto reconstructed = scl::details::SecretFromPointAndShares(x_point, shares);
    REQUIRE(reconstructed == secret);
  }

  SECTION("EvPolyFromSecretsAndDegree") {
    Vec secrets{FF(123), FF(456), FF(789)};
    auto poly = scl::details::EvPolyFromSecretsAndDegree(secrets, degree, prg);
    auto shares = scl::details::SharesFromEvPoly(poly, n_shares);

    REQUIRE(shares.Size() == n_shares);

    auto reconstructed = scl::details::SecretsFromSharesAndLength(shares, secrets.Size());
    REQUIRE(reconstructed.Equals(secrets));
  }  
  
  SECTION("EvPolyFromSecretsAndPoints") {
    Vec secrets{FF(123), FF(456), FF(789)};
    Vec x_points{FF(32), FF(23), FF(57), FF(123), FF(124),\
      FF(125), FF(126), FF(127), FF(128), FF(129)};
    Vec x_secrets{FF(32), FF(23), FF(57)};
    auto poly = scl::details::EvPolyFromSecretsAndPoints(secrets, x_points, prg);
    auto shares = scl::details::SharesFromEvPoly(poly, n_shares);

    REQUIRE(shares.Size() == n_shares);

    auto reconstructed = scl::details::SecretsFromPointsAndShares(x_secrets, shares);
    REQUIRE(reconstructed.Equals(secrets));
  }  

}
