#ifndef MULT_H
#define MULT_H

#include "tp.h"

namespace tp {

  struct MultLambdas {
    FF l_alpha;
    FF l_beta;
    FF l_gamma;
    FF l_alpha_x_beta;
  };

  struct MuAB {
    Vec mu_alpha;
    Vec mu_beta;
  };

  struct ShrMuAB {
    FF mu_alpha;
    FF mu_beta;
  };
  
  class Mult {
  public:
    Mult(scl::Network& network, std::size_t id, std::size_t packing) : mNetwork(network), mID(id), mParties(network.Size()), mPacking(packing), mCount(0) {};

    void Prepare(const MultLambdas mult_lambdas) {
      mLambdas.emplace_back(mult_lambdas);
      if ( mID == 0 ) {
	// P1 prepares the mu vectors in the right order
	// we assume the mu is zero
	Vec zero_mu(mPacking);
	MuAB muab{zero_mu, zero_mu};
	mMuAB.emplace_back(muab);
      }
      mCount++;
    };

    void ReceiveFromP1() {
      if ( mID == 0 ) {
	for (auto muab : mMuAB) {
	  // 1. Generate shares
	  auto poly_A = scl::details::EvPolyFromSecretsAndDegree(muab.mu_alpha, mPacking-1, mPRG);
	  Vec shares_A = scl::details::SharesFromEvPoly(poly_A, mParties);

	  auto poly_B = scl::details::EvPolyFromSecretsAndDegree(muab.mu_beta, mPacking-1, mPRG);
	  Vec shares_B = scl::details::SharesFromEvPoly(poly_B, mParties);

	  // 2. Send the shares
	  for (std::size_t i = 0; i < mParties; ++i) {
	  mNetwork.Party(i)->Send(shares_A[i]);
	  mNetwork.Party(i)->Send(shares_B[i]);
	  }
	}

      }
      // Receive shares
      for (std::size_t i = 0; i < mCount; i++) {
	FF shr_mu_A;
	FF shr_mu_B;
	mNetwork.Party(0)->Recv(shr_mu_A);
	mNetwork.Party(0)->Recv(shr_mu_B);
	ShrMuAB shr_muAB{shr_mu_A, shr_mu_B};
	mShrMuAB.emplace_back(shr_muAB);
      }
    };

    void LocalComputation() {
      for (std::size_t i = 0; i < mCount; i++) {
	FF shr_mu_C;
	shr_mu_C = mShrMuAB[i].mu_beta * mLambdas[i].l_alpha + mShrMuAB[i].mu_alpha * mLambdas[i].l_beta \
	  + mShrMuAB[i].mu_alpha * mShrMuAB[i].mu_beta + mLambdas[i].l_alpha_x_beta - mLambdas[i].l_gamma;
	mShrMuGamma.Emplace(shr_mu_C);
      }
    };

    void SendToP1() {
      // Send shares to P1
      for (auto shr_mu_C : mShrMuGamma) {
	mNetwork.Party(0)->Send(shr_mu_C);
      }

      // P1 receives these shares
      if ( mID == 0 ) {
	for (std::size_t j = 0; j < mCount; ++j) {
	  Vec mu_gamma;
	  Vec shares;
	  for (std::size_t i = 0; i < mParties; ++i) {
	    FF shr_mu_C;
	    mNetwork.Party(i)->Recv(shr_mu_C);
	    shares.Emplace(shr_mu_C);
	  }
	  mu_gamma = scl::details::SecretsFromSharesAndLength(shares, mPacking);
	  mMuGamma.emplace_back(mu_gamma);
	}
      }
    };
    
  private:

    scl::Network mNetwork;
    std::size_t mID;
    std::size_t mParties;
    std::size_t mPacking;
    std::size_t mCount;

    scl::PRG mPRG;
    
    // Preprocessed shares
    vec<MultLambdas> mLambdas;

    // mu_alpha and mu_beta, held by P1
    vec<MuAB> mMuAB;
    // Shares of mu_alpha and mu_beta
    vec<ShrMuAB> mShrMuAB;

    // mu_gamma, held by P1
    vec<Vec> mMuGamma;
    // Shares of mu_gamma
    Vec mShrMuGamma;

  };
  
} // namespace tp

#endif  // MULT_H
