/**
 * @file evpoly.h
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
#ifndef _SCL_SS_EVPOLY_H
#define _SCL_SS_EVPOLY_H

#include <array>
#include <stdexcept>

#include "scl/math/vec.h"
#include "scl/ss/poly.h"

namespace scl {
  namespace details {

    /**
     * @brief Polynomials over finite fields, in evaluation representation.
     * 
     * @details A polynomial of degree d can be represented as a vector of
     * length d+1 containing its coefficients, but alternatively, it can
     * also be represented by a vector of its evaluations at d+1
     * points. This is the representation we make use of here.
     */
    template <typename T>
    class EvPolynomial {
    public:
      /**
       * @brief Construct a constant polynomial with constant term 0.
       */
      EvPolynomial() : mX{T(0)}, mY{T(0)}  {};

      /**
       * @brief Construct a constant polynomial.
       * @param constant the constant term of the polynomial
       */
      EvPolynomial(const T& constant) : mX{T(0)}, mY({constant}) {};

      /**
       * @brief Construct a polynomial from a list of x and y points.
       * @param x_points the set of evaluation points
       * @param y_points the set of evaluations
       */
      EvPolynomial(const Vec<T>& x_points, const Vec<T>& y_points) {
	if ((x_points.Size() != y_points.Size()))
	  throw std::invalid_argument("number of evaluation points and evaluations do not match");
	if ((x_points.Size() == 0))
	  throw std::invalid_argument("empty set cannot be used for initialization");
	mX = x_points;
	mY = y_points;
      };

      /**
       * @brief Construct a polynomial from a list of y points and a
       * starting point x. The x points are taken consecutive after this one.
       * @param x_start the first evaluation point
       * @param y_points the set of evaluations
       */
      EvPolynomial(const T& x_start, const Vec<T>& y_points) {
	if ((y_points.Size() == 0))
	  throw std::invalid_argument("empty set cannot be used for initialization");
	Vec<T> x_points;
	x_points.Reserve(y_points.Size());
	for (std::size_t i = 0; i < y_points.Size(); i++){
	  x_points.Emplace(x_start + T(i));
	};
	mX = x_points;
	mY = y_points;
      };

      /**
       * @brief Construct a polynomial from a list of y points. The
       * starting point is taken to be 0
       * @param y_points the set of evaluations
       */
      EvPolynomial(const Vec<T>& y_points) : EvPolynomial<T>(T(0), y_points){};

      /**
       * @brief Evaluate this polynomial on a supplied point.
       * @param x the point to evaluate this polynomial on
       * @return f(x) where \p x is the supplied point and f this polynomial.
       */
      T Evaluate(const T& x) const {
	T z;
	for (std::size_t j = 0; j < Degree()+1; ++j) {
	  T ell(1);
	  auto xj = mX[j];
	  for (std::size_t m = 0; m < Degree()+1; ++m) {
	    if (m == j) continue;
	    auto xm = mX[m];
	    ell *= (x - xm) / (xj - xm);
	  }
	  z += mY[j] * ell;
	}
	return z;
      };

      /**
       * @brief Evaluate this polynomial on a supplied list of points.
       * @param points points to evaluate on
       * @return f(x) for x in points
       */
      Vec<T> Evaluate(const Vec<T>& points) const {
	Vec<T> output;
	output.Reserve(points.Size());
	for(const auto& point : points) {
	  output.Emplace(Evaluate(point));
	}
	return output;
      };

      Vec<T> GetY() const { return mY; }
      Vec<T> GetX() const { return mX; }

      /**
       * @brief Add two polynomials.
       */
      EvPolynomial Add(const EvPolynomial& p) const;

      /**
       * @brief Subtraction two polynomials.
       */
      EvPolynomial Subtract(const EvPolynomial& p) const;

      /**
       * @brief Multiply two polynomials.
       */
      EvPolynomial Multiply(const EvPolynomial& p) const;

      /**
       * @brief Returns true if this is the 0 polynomial.
       */
      bool IsZero() const { return Degree() == 0 && mY[0] == T(0); };

      /**
       * @brief Degree of this polynomial.
       */
      std::size_t Degree() const { return mY.Size() - 1; };

      /**
       * @brief First evaluation point in the list. Useful when the points
       * are consecutive
       */
      T GetFirstPoint() const { return mX[0]; };

      // /**
      //  * @brief Get polynomial representation. TODO
      //  */
      // Polynomial<T> CoefficientRepresentation() const;

      /**
       * @brief Access evaluations
       */
      T& operator[](std::size_t idx) { return mY[idx]; };

      /**
       * @brief Access evaluations
       */
      T operator[](std::size_t idx) const { return mY[idx]; };


    private:
      Vec<T> mX;
      Vec<T> mY;
    };

    template <typename T>
    EvPolynomial<T> EvPolynomial<T>::Add(const EvPolynomial<T>& q) const {
      if (!(mX.Equals(q.GetX())))
	throw std::invalid_argument("cannot add evpolys with different x points");
      const auto c = mY.Add(q.GetY());
      return EvPolynomial<T>(mX, c);
    }

    template <typename T>
    EvPolynomial<T> EvPolynomial<T>::Subtract(const EvPolynomial<T>& q) const {
      if (!(mX.Equals(q.GetX())))
	throw std::invalid_argument("cannot subtract evpolys with different x points");
      const auto c = mY.Subtract(q.GetY());
      return EvPolynomial<T>(mX, c);
    }

  }  // namespace details
}  // namespace scl

#endif /* _SCL_SS_EVPOLY_H */
