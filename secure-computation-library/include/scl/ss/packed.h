/**
 * @file packed.h
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
#ifndef _SCL_SS_PACKED_H
#define _SCL_SS_PACKED_H

#include <array>
#include <iostream>
#include <stdexcept>

#include "scl/math/la.h"
#include "scl/math/vec.h"
#include "scl/prg.h"
#include "scl/ss/poly.h"
#include "scl/ss/evpoly.h"

namespace scl {
  namespace details {

    /**
     * @brief Creates an evpoly from a vector of secrets
     * @param secrets the vector of secrets
     * @param x_points the evaluation points. The first secret.Size()
     * points are used for the secrets
     * @param prg pseudorandom function for randomness
     * @return A polynomial
     */
    template <typename T>
    EvPolynomial<T> EvPolyFromSecretsAndPoints(const Vec<T>& secrets, const Vec<T>& x_points, PRG& prg) {
      if (secrets.Size() > x_points.Size())
	throw std::invalid_argument("number of secrets is larger than number of x points");
      std::size_t n_secrets = secrets.Size();
      std::size_t n_points = x_points.Size();

      // Populate y_points with random entries for indexes above n_secrets
      Vec<T> y_points = Vec<T>::PartialRandom(
					      n_points, [n_secrets](std::size_t i) { return i >= n_secrets; }, prg);
      for (std::size_t i = 0; i < n_secrets; i++) {
	y_points[i] = secrets[i];
      }

      auto poly = EvPolynomial<T>(x_points, y_points);
      return poly;
    }

    /**
     * @brief Creates an evpoly from a vector of secrets, putting them
     * in positions [0,-1,...]
     * @param secrets the vector of secrets
     * @param degree the desired degree
     * @param prg pseudorandom function for randomness
     * @return A polynomial
     */
    template <typename T>
    EvPolynomial<T> EvPolyFromSecretsAndDegree(const Vec<T>& secrets, std::size_t degree, PRG& prg) {
      Vec<T> x_points;
      x_points.Reserve(degree+1);
      for (std::size_t i = 0; i < secrets.Size(); i++) { x_points.Emplace(T(-i)); }
      for (std::size_t i = 0; i < (degree + 1) - secrets.Size(); i++) { x_points.Emplace(T(i+1)); }

      return EvPolyFromSecretsAndPoints(secrets, x_points, prg);
    }    

    /**
     * @brief Creates an evpoly from a single secret
     * @param secret the secret
     * @param x_point the x point for the secret
     * @param prg pseudorandom function for randomness
     * @return A polynomial
     */
    template <typename T>
    EvPolynomial<T> EvPolyFromSecretAndPointAndDegree(const T& secret, const T& x_point, std::size_t degree, PRG& prg) {
      Vec<T> x_points;
      x_points.Reserve(degree+1);
      x_points.Emplace(x_point);
      for (std::size_t i = 0; i < degree; i++) { x_points.Emplace(T(i+1)); }
      Vec<T> secrets{secret};
      return EvPolyFromSecretsAndPoints(secrets, x_points, prg);
    }

    /**
     * @brief Creates an evpoly from a single secret at position 0
     * @param secret the secret
     * @param prg pseudorandom function for randomness
     * @return A polynomial
     */
    template <typename T>
    EvPolynomial<T> EvPolyFromSecretAndDegree(const T& secret, std::size_t degree, PRG& prg) {
      T x_point(0);
      return EvPolyFromSecretAndPointAndDegree(secret, x_point, degree, prg);
    }

    /**
     * @brief Returns the shares f(1)...f(n) of a polynomial f
     * @param poly the ev polynomial
     * @param n_shares how many shares to return
     * @return A vector of shares
     */
    template <typename T>
    Vec<T> SharesFromEvPoly(const EvPolynomial<T>& poly, std::size_t n_shares) {
      Vec<T> vec;
      vec.Reserve(n_shares);
      for (std::size_t i = 0; i < n_shares; i++) { vec.Emplace(T(i+1)); }
      return poly.Evaluate(vec);
    }

    /**
     * @brief Returns the polynomial f of degree n-1 given by the
     * shares f(1)...f(n)
     * @param shares the shares. The ev points are 1...n
     * @return An EvPolynomial
     */
    template <typename T>
    EvPolynomial<T> EvPolyFromShares(const Vec<T>& shares) {
      Vec<T> x_points;
      x_points.Reserve(shares.Size());
      for (std::size_t i = 0; i < shares.Size(); i++) { x_points.Emplace(T(i+1)); }
      return EvPolynomial(x_points, shares);
    }

    /**
     * @brief Reconstructs the secrets given shares
     * @param x_secrets x points for the secrets
     * @param shares the shares. The ev points are 1...n
     * @return A vector with the secrets
     */
    template <typename T>
    Vec<T> SecretsFromPointsAndShares(const Vec<T>& x_secrets, const Vec<T>& shares) {
      auto poly = EvPolyFromShares(shares);
      return poly.Evaluate(x_secrets);
    }

    /**
     * @brief Reconstructs a vector of secrets given shares. The
     * secrets are assumed to be in positions [0,-1,...-length]
     * @param shares the shares. The ev points are 1...n
     * @param length length of the secret
     * @return The secrets
     */
    template <typename T>
    Vec<T> SecretsFromSharesAndLength(const Vec<T>& shares, std::size_t length) {
      Vec<T> x_secrets;
      x_secrets.Reserve(length);
      for (std::size_t i = 0; i < length; i++) { x_secrets.Emplace(T(-i)); }      
      return SecretsFromPointsAndShares(x_secrets, shares);
    }    

    /**
     * @brief Reconstructs a single secret given shares
     * @param x_secret x point for the secret
     * @param shares the shares. The ev points are 1...n
     * @return The secret
     */
    template <typename T>
    T SecretFromPointAndShares(const T& x_secret, const Vec<T>& shares) {
      auto poly = EvPolyFromShares(shares);
      return poly.Evaluate(x_secret);
    }

    /**
     * @brief Reconstructs a secret given shares. The
     * secret is assumed to be in position 0
     * @param shares the shares. The ev points are 1...n
     * @return The secret
     */
    template <typename T>
    T SecretFromShares(const Vec<T>& shares) {
      return SecretFromPointAndShares(T(0), shares);
    }



  }  // namespace details
}  // namespace scl

#endif  // _SCL_SS_PACKED_H
