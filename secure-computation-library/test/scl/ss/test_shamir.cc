#include <catch2/catch.hpp>

#include "../gf7.h"
#include "scl/math/ff.h"
#include "scl/math/vec.h"
#include "scl/prg.h"
#include "scl/ss/shamir.h"

TEST_CASE("Shamir", "[ss]") {
  using FF = scl::FF<61>;
  using Vec = scl::Vec<FF>;

  scl::PRG prg;

  SECTION("Share") {
    auto secret = FF(123);
    Vec alphas = {FF(2), FF(5), FF(3)};
    auto share_poly = scl::details::CreateShamirSharePolynomial(secret, 2, prg);
    auto shares = scl::CreateShamirShares(share_poly, alphas);

    auto reconstructed =
        scl::ReconstructShamirPassive(shares, alphas, FF(0), 2);
    REQUIRE(reconstructed == secret);

    auto some_share = scl::ReconstructShamirPassive(shares, alphas, FF(3), 2);
    REQUIRE(some_share == shares[2]);
  }

  SECTION("Passive") {
    auto secret = FF(123);
    auto shares = scl::CreateShamirShares(secret, 4, 3, prg);
    auto reconstructed = scl::ReconstructShamirPassive(shares, 3);

    REQUIRE(reconstructed == secret);

    REQUIRE_THROWS_MATCHES(
        scl::ReconstructShamirPassive(shares, 4), std::invalid_argument,
        Catch::Matchers::Message("not enough shares to reconstruct"));

    Vec alphas = {FF(1), FF(2), FF(3)};
    REQUIRE_THROWS_MATCHES(
        scl::ReconstructShamirPassive(shares, alphas, FF(0), 3),
        std::invalid_argument,
        Catch::Matchers::Message("not enough alphas to reconstruct"));
  }

  SECTION("Detection") {
    auto secret = FF(123);
    auto shares = scl::CreateShamirShares(secret, 7, 3, prg);
    auto reconstructed = scl::ReconstructShamir(shares, 3);
    REQUIRE(reconstructed == secret);

    auto shares0 = shares;
    shares0[2] = FF(4);
    REQUIRE_THROWS_MATCHES(
        scl::ReconstructShamir(shares0, 3), std::logic_error,
        Catch::Matchers::Message("error detected during reconstruction"));

    auto shares1 = shares;
    shares1[6] = FF(3);
    REQUIRE_THROWS_MATCHES(
        scl::ReconstructShamir(shares1, 3), std::logic_error,
        Catch::Matchers::Message("error detected during reconstruction"));
  }

  SECTION("Correction") {
    // no errors
    auto secret = FF(123);
    auto shares = scl::CreateShamirShares(secret, 7, 2, prg);
    auto reconstructed = scl::ReconstructShamirRobust(shares, 2);
    CHECK(reconstructed == secret);

    // one error
    shares[0] = FF(63212);
    auto reconstructed_1 = scl::ReconstructShamirRobust(shares, 2);
    CHECK(reconstructed_1 == secret);

    // two errors
    shares[2] = FF(63212211);
    auto reconstructed_2 = scl::ReconstructShamirRobust(shares, 2);
    CHECK(reconstructed_2 == secret);

    // three errors -- that's one too many
    shares[1] = FF(123);
    REQUIRE_THROWS_MATCHES(
        scl::ReconstructShamirRobust(shares, 2), std::logic_error,
        Catch::Matchers::Message("could not correct shares"));
  }
}

TEST_CASE("BerlekampWelch", "[ss][math]") {
  // https://en.wikipedia.org/wiki/Berlekamp%E2%80%93Welch_algorithm#Example

  using FF = scl::details::FF<0, scl::details::GF7>;
  using Vec = scl::Vec<FF>;

  Vec bs = {FF(1), FF(5), FF(3), FF(6), FF(3), FF(2), FF(2)};
  Vec as = {FF(0), FF(1), FF(2), FF(3), FF(4), FF(5), FF(6)};
  Vec corrected = {FF(1), FF(6), FF(3), FF(6), FF(1), FF(2), FF(2)};

  auto pe = scl::ReconstructShamirRobust(bs, as, 2);
  auto p = pe[0];
  auto e = pe[1];

  // errors
  REQUIRE(e.Evaluate(FF(1)) == FF{});
  REQUIRE(e.Evaluate(FF(4)) == FF{});

  for (std::size_t i = 0; i < as.Size(); ++i) {
    REQUIRE(p.Evaluate(as[i]) == corrected[i]);
  }
}
