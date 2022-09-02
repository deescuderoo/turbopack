#include <catch2/catch.hpp>

#include "scl/math/ff.h"
#include "scl/prg.h"
#include "scl/ss/evpoly.h"

using FF = scl::FF<61>;
using EvPoly = scl::details::EvPolynomial<FF>;
using Vec = scl::Vec<FF>;

TEST_CASE("Evaluation Polynomials", "[ss][math]") {
  SECTION("DefaultConstruct") {
    EvPoly p;
    REQUIRE(p.Degree() == 0);
    REQUIRE(p[0] == FF());
    REQUIRE(p.IsZero());
  }

  SECTION("ConstantConstruct") {
    EvPoly p(FF(123));
    REQUIRE(p.Degree() == 0);
    REQUIRE(p[0] == FF(123));
  }

  SECTION("EvaluationsConstruct") {
    scl::Vec evals = {FF(1), FF(2), FF(6)};
    EvPoly p = EvPoly(evals);
    REQUIRE(p.Degree() == 2);
    REQUIRE(p[0] == FF(1));
    REQUIRE(p[1] == FF(2));
    REQUIRE(p[2] == FF(6));
  }

  // f(x) = 1 + x + x^2
  // f(-1) = 1, f(0) = 1, f(1) = 3, f(2) = 7
  SECTION("PointsAndEvaluationsConstruct") {
    scl::Vec x_points = {FF(-1), FF(1), FF(2)};
    scl::Vec y_points = {FF(1), FF(3), FF(7)};
    EvPoly p = EvPoly(x_points, y_points);
    REQUIRE(p.Degree() == 2);
    REQUIRE(p[0] == FF(1));
    REQUIRE(p[1] == FF(3));
    REQUIRE(p[2] == FF(7));
  }

  SECTION("StartAndEvaluationsConstruct") {
    FF x_start = FF(-1);
    scl::Vec y_points = {FF(1), FF(0), FF(3)};
    EvPoly p = EvPoly(x_start, y_points);
    REQUIRE(p.Degree() == 2);
    REQUIRE(p[0] == FF(1));
    REQUIRE(p[1] == FF(0));
    REQUIRE(p[2] == FF(3));
  }

  SECTION("OnlyEvaluationsConstruct") {
    scl::Vec y_points = {FF(1), FF(0), FF(3)};
    EvPoly p = EvPoly(y_points);
    REQUIRE(p.Degree() == 2);
    REQUIRE(p[0] == FF(1));
    REQUIRE(p[1] == FF(0));
    REQUIRE(p[2] == FF(3));
  }  

  SECTION("Evaluate") {
    scl::Vec evals012 = {FF(1), FF(3), FF(7)};
    EvPoly p = EvPoly(evals012);
    REQUIRE(p.Evaluate(FF(-1)) == FF(1));
    REQUIRE(p.Evaluate(FF(1)) == FF(3));

    scl::Vec evalsm101 = {FF(1), FF(1), FF(3)};
    auto start = FF(-1);
    EvPoly q = EvPoly(start, evalsm101);
    REQUIRE(q.Evaluate(FF(2)) == FF(7));
  }

  SECTION("Add") {
    scl::Vec c0 = {FF(1), FF(2), FF(3)};
    scl::Vec c1 = {FF(5), FF(3), FF(3)};
    auto p = EvPoly(c0);
    auto q = EvPoly(c1);
    auto e = p.Add(q);
    REQUIRE(e.Degree() == q.Degree());
    REQUIRE(e[0] == FF(6));
    REQUIRE(e[1] == FF(5));
    REQUIRE(e[2] == FF(6));

    auto d = q.Add(p);
    REQUIRE(d.Degree() == q.Degree());
    REQUIRE(d[0] == e[0]);
    REQUIRE(d[1] == e[1]);
    REQUIRE(d[2] == e[2]);

    scl::Vec cn = {-FF(1), -FF(2), -FF(3)};
    auto t = EvPoly(cn);
    auto w = t.Add(p);
    REQUIRE(w[0] == FF(0));
    REQUIRE(w[1] == FF(0));
    REQUIRE(w[2] == FF(0));
  }

  SECTION("Subtract") {
    scl::Vec c0 = {FF(1), FF(2), FF(3)};
    scl::Vec c1 = {FF(5), FF(3), FF(3)};
    auto p = EvPoly(c0);
    auto q = EvPoly(c1);
    auto e = p.Subtract(q);
    REQUIRE(e.Degree() == q.Degree());
    REQUIRE(e[0] == FF(-4));
    REQUIRE(e[1] == FF(-1));
    REQUIRE(e[2] == FF(0));
  }
}
