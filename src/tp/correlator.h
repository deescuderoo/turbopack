#ifndef CORRELATOR_H
#define CORRELATOR_H

#include <catch2/catch.hpp>
#include <iostream>
#include <map>
#include <assert.h>

#include "tp.h"
#include "mult_gate.h"
#include "input_gate.h"
#include "output_gate.h"

namespace tp {
  struct MultBatchFIPrep {
    // Triples
    FF mShrA;
    FF mShrB;
    FF mShrC;

    // Shares of 0
    FF mShrO1;
    FF mShrO2;
    FF mShrO3;

    MultBatchFIPrep(FF lambda) {
      mShrA = lambda;
      mShrB = lambda;
      mShrC = lambda*lambda;
      mShrO1 = FF(0);
      mShrO2 = FF(0);
      mShrO3 = FF(0);
    }
    MultBatchFIPrep() {
      mShrA = FF(0);
      mShrB = FF(0);
      mShrC = FF(0);
      mShrO1 = FF(0);
      mShrO2 = FF(0);
      mShrO3 = FF(0);
    }
  };

  struct IOBatchFIPrep {
    // Double sharings
    FF mShrsR;
    FF mShrsRd;

    IOBatchFIPrep(FF lambda) {
      mShrsR = lambda;
      mShrsRd = lambda;
    }
    IOBatchFIPrep() {
      mShrsR = FF(0);
      mShrsRd = FF(0);
    }
  };  

  class Correlator {
  public:
    Correlator() {};

    Correlator(std::size_t n_ind_shares, std::size_t n_mult_batches, std::size_t n_inout_batches, std::size_t batch_size) :
      mNIndShrs(n_ind_shares), mNMultBatches(n_mult_batches), mNInOutBatches(n_inout_batches), \
      mCTRIndShrs(0), mCTRMultBatches(0), mCTRInOutBatches(0), mBatchSize(batch_size) {

      mIndShrs.reserve(mNIndShrs);
      mMultBatchFIPrep.reserve(mNMultBatches);
      mIOBatchFIPrep.reserve(mNInOutBatches);
    }

    void PopulateDummy(FF lambda) {
      mIndShrs = std::vector<FF>(mNIndShrs, lambda);
      mMultBatchFIPrep = std::vector<MultBatchFIPrep>(mNMultBatches, MultBatchFIPrep(lambda));
      mIOBatchFIPrep = std::vector<IOBatchFIPrep>(mNInOutBatches, IOBatchFIPrep(lambda));
    }

    void PopulateDummy() {
      mIndShrs = std::vector<FF>(mNIndShrs, FF(0));
      mMultBatchFIPrep = std::vector<MultBatchFIPrep>(mNMultBatches, MultBatchFIPrep());
      mIOBatchFIPrep = std::vector<IOBatchFIPrep>(mNInOutBatches, IOBatchFIPrep());
    }

    // TODO actual protocol to get the prep
    void Populate();

    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network->Size();
    }

    void PopulateIndvShrs(std::shared_ptr<MultGate> gate) {
      if ( !gate->IsPadding() ) mMapIndShrs[gate] = mIndShrs[mCTRIndShrs++];
    }
    void PopulateIndvShrs(std::shared_ptr<InputGate> gate) {
      if ( !gate->IsPadding() ) mMapIndShrs[gate] = mIndShrs[mCTRIndShrs++];
    }
    void PopulateIndvShrs(std::shared_ptr<AddGate> gate) {
      // This is done in topological order, so previous (left&right) gates are already handled
      mMapIndShrs[gate] = mMapIndShrs[gate->GetLeft()] + mMapIndShrs[gate->GetRight()];
    }
    void PopulateIndvShrs(std::shared_ptr<OutputGate> gate) {
      mMapIndShrs[gate] = mMapIndShrs[gate->GetLeft()];
    }

    void PopulateMultBatches(std::shared_ptr<MultBatch> mult_batch) {
      mMapMultBatch[mult_batch] = mMultBatchFIPrep[mCTRMultBatches++];
    }

    void PopulateInputBatches(std::shared_ptr<InputBatch> input_batch) {
      mMapInputBatch[input_batch] = mIOBatchFIPrep[mCTRInOutBatches++];
    }

    void PopulateOutputBatches(std::shared_ptr<OutputBatch> output_batch) {
      mMapOutputBatch[output_batch] = mIOBatchFIPrep[mCTRInOutBatches++];
    }

    // PREP INPUT BATCH
    void PrepInputPartiesSendP1(std::shared_ptr<InputBatch> input_batch) {
    // 1 collect [lambda_alpha]_n-1
      FF shr_lambdaA_p_R(0);
      for (std::size_t i = 0; i < mBatchSize; i++) {
	shr_lambdaA_p_R += mSharesOfEi[i] * mMapIndShrs[input_batch->GetInputGate(i)];
      }

    // 2 get random sharing [r]_n-1 and add [lambda_alpha]_n-1 + [r]_n-1
      shr_lambdaA_p_R += mMapInputBatch[input_batch].mShrsRd;

    // 3 send to P1
      mNetwork->Party(0)->Send(shr_lambdaA_p_R);
    }
    
    void PrepInputP1ReceivesAndSends() {
      if (mID == 0) {
	Vec recv_shares;
	Vec recv_secret;
	recv_shares.Reserve(mParties);
	recv_secret.Reserve(mBatchSize);

	// P1 receives
	for (std::size_t i = 0; i < mParties; i++) {
	  FF buffer;
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares.Emplace(buffer);
	}
	recv_secret = scl::details::SecretsFromSharesAndLength(recv_shares, mBatchSize);

	// P1 generates new shares
	auto poly = scl::details::EvPolyFromSecretsAndDegree(recv_secret, mBatchSize-1, mPRG);
	Vec new_shares = scl::details::SharesFromEvPoly(poly, mParties);

	// P1 sends
	for (std::size_t i = 0; i < mParties; ++i) {
	  mNetwork->Party(i)->Send(new_shares[i]);
	}
      }
    }

    void PrepInputPartiesReceive(std::shared_ptr<InputBatch> input_batch) {
      // Receive
      FF recv_share;
      mNetwork->Party(0)->Recv(recv_share);

      // Subtract shares of [r]_n-k
      FF new_share = recv_share - mMapInputBatch[input_batch].mShrsR;

      // Set preprocessing
      input_batch->SetPreprocessing(new_share);
    };

    void PrepInputPartiesSendOwner(std::shared_ptr<InputBatch> input_batch) {
      mNetwork->Party(input_batch->GetOwner())->Send(input_batch->GetPackedShrLambda());
    }

    void PrepInputOwnerReceives(std::shared_ptr<InputBatch> input_batch) {
      if (mID == input_batch->GetOwner()) {
	Vec recv_shares;
	Vec recv_secret;
	recv_shares.Reserve(mParties);
	recv_secret.Reserve(mBatchSize);

	// Owner receives
	for (std::size_t i = 0; i < mParties; i++) {
	  FF buffer;
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares.Emplace(buffer);
	}
	// TODO watch out for degree
	recv_secret = scl::details::SecretsFromSharesAndLength(recv_shares, mBatchSize);

	// Assign lambdas
	for (std::size_t i = 0; i < mBatchSize; i++) {
	  auto input_gate = input_batch->GetInputGate(i);
	  input_gate->SetLambda(recv_secret[i]);
	}
      }
    }

    void PrepOutputPartiesSendOwner(std::shared_ptr<OutputBatch> output_batch) {
      mNetwork->Party(output_batch->GetOwner())->Send(output_batch->GetPackedShrLambda());
    }

    void PrepOutputOwnerReceives(std::shared_ptr<OutputBatch> output_batch) {
      if (mID == output_batch->GetOwner()) {
	Vec recv_shares;
	Vec recv_secret;
	recv_shares.Reserve(mParties);
	recv_secret.Reserve(mBatchSize);

	// Owner receives
	for (std::size_t i = 0; i < mParties; i++) {
	  FF buffer;
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares.Emplace(buffer);
	}
	// TODO watch out for degree
	recv_secret = scl::details::SecretsFromSharesAndLength(recv_shares, mBatchSize);

	// Assign lambdas
	for (std::size_t i = 0; i < mBatchSize; i++) {
	  auto output_gate = output_batch->GetOutputGate(i);
	  output_gate->SetLambda(recv_secret[i]);
	}
      }
    }



    // PREP OUTPUT BATCH
    void PrepOutputPartiesSendP1(std::shared_ptr<OutputBatch> output_batch) {
    // 1 collect [lambda_alpha]_n-1
      FF shr_lambdaA_p_R(0);
      for (std::size_t i = 0; i < mBatchSize; i++) {
	// assert(mMapIndShrs[output_batch->GetOutputGate(i)] != FF(0));
	shr_lambdaA_p_R += mSharesOfEi[i] * mMapIndShrs[output_batch->GetOutputGate(i)];
      }

    // 2 get random sharing [r]_n-1 and add [lambda_alpha]_n-1 + [r]_n-1
      shr_lambdaA_p_R += mMapOutputBatch[output_batch].mShrsRd;

    // 3 send to P1
      mNetwork->Party(0)->Send(shr_lambdaA_p_R);
    }
    
    void PrepOutputP1ReceivesAndSends() {
      if (mID == 0) {
	Vec recv_shares;
	Vec recv_secret;
	recv_shares.Reserve(mParties);
	recv_secret.Reserve(mBatchSize);

	// P1 receives
	for (std::size_t i = 0; i < mParties; i++) {
	  FF buffer;
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares.Emplace(buffer);
	}
	recv_secret = scl::details::SecretsFromSharesAndLength(recv_shares, mBatchSize);

	// P1 generates new shares
	auto poly = scl::details::EvPolyFromSecretsAndDegree(recv_secret, mBatchSize-1, mPRG);
	Vec new_shares = scl::details::SharesFromEvPoly(poly, mParties);

	// P1 sends
	for (std::size_t i = 0; i < mParties; ++i) {
	  mNetwork->Party(i)->Send(new_shares[i]);
	}
      }
    }

    void PrepOutputPartiesReceive(std::shared_ptr<OutputBatch> output_batch) {
      // Receive
      FF recv_share;
      mNetwork->Party(0)->Recv(recv_share);

      // Subtract shares of [r]_n-k
      FF new_share = recv_share - mMapOutputBatch[output_batch].mShrsR;

      // Set preprocessing
      output_batch->SetPreprocessing(new_share);
    }

    // PREP MULT BATCH
    void PrepMultPartiesSendP1(std::shared_ptr<MultBatch> mult_batch) {
    // 1 collect [lambda_alpha]_n-1
      FF shr_lambdaA_p_R(0);
      FF shr_lambdaB_p_R(0);
      for (std::size_t i = 0; i < mBatchSize; i++) {
	shr_lambdaA_p_R += mSharesOfEi[i] * mMapIndShrs[mult_batch->GetMultGate(i)->GetLeft()];
	shr_lambdaB_p_R += mSharesOfEi[i] * mMapIndShrs[mult_batch->GetMultGate(i)->GetRight()];
      }

    // 2 get random sharing [r]_n-1 and add [lambda_alpha]_n-1 + [r]_n-1
      shr_lambdaA_p_R += mMapMultBatch[mult_batch].mShrA + mMapMultBatch[mult_batch].mShrO1;
      shr_lambdaB_p_R += mMapMultBatch[mult_batch].mShrB + mMapMultBatch[mult_batch].mShrO2;

    // 3 send to P1
      mNetwork->Party(0)->Send(shr_lambdaA_p_R);
      mNetwork->Party(0)->Send(shr_lambdaB_p_R);
    }
    
    void PrepMultP1ReceivesAndSends() {
      if (mID == 0) {
	Vec recv_shares_A;
	Vec recv_secret_A;
	Vec recv_shares_B;
	Vec recv_secret_B;
	recv_shares_A.Reserve(mParties);
	recv_secret_A.Reserve(mBatchSize);
	recv_shares_B.Reserve(mParties);
	recv_secret_B.Reserve(mBatchSize);

	// P1 receives
	for (std::size_t i = 0; i < mParties; i++) {
	  FF buffer;
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares_A.Emplace(buffer);
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares_B.Emplace(buffer);
	}
	recv_secret_A = scl::details::SecretsFromSharesAndLength(recv_shares_A, mBatchSize);
	recv_secret_B = scl::details::SecretsFromSharesAndLength(recv_shares_B, mBatchSize);

	// P1 generates new shares
	auto poly_A = scl::details::EvPolyFromSecretsAndDegree(recv_secret_A, mBatchSize-1, mPRG);
	auto poly_B = scl::details::EvPolyFromSecretsAndDegree(recv_secret_B, mBatchSize-1, mPRG);
	Vec new_shares_A = scl::details::SharesFromEvPoly(poly_A, mParties);
	Vec new_shares_B = scl::details::SharesFromEvPoly(poly_B, mParties);

	// P1 sends
	for (std::size_t i = 0; i < mParties; ++i) {
	  mNetwork->Party(i)->Send(new_shares_A[i]);
	  mNetwork->Party(i)->Send(new_shares_B[i]);
	}
      }
    }

    void PrepMultPartiesReceive(std::shared_ptr<MultBatch> mult_batch) {
      // Receive
      FF recv_share_A;
      FF recv_share_B;
      mNetwork->Party(0)->Recv(recv_share_A);
      mNetwork->Party(0)->Recv(recv_share_B);

      // Subtract shares of [r]_n-k
      FF new_share_A = recv_share_A - mMapMultBatch[mult_batch].mShrA;
      FF new_share_B = recv_share_B - mMapMultBatch[mult_batch].mShrB;

      // Set deltas
      FF shr_delta(0);
      for (std::size_t i = 0; i < mBatchSize; i++) {
	shr_delta -= mSharesOfEi[i] * mMapIndShrs[mult_batch->GetMultGate(i)];
      }
      shr_delta += recv_share_A * recv_share_B - recv_share_A * mMapMultBatch[mult_batch].mShrB \
	- recv_share_B * mMapMultBatch[mult_batch].mShrA + mMapMultBatch[mult_batch].mShrC \
	+ mMapMultBatch[mult_batch].mShrO3;

      // Set preprocessing
      mult_batch->SetPreprocessing(new_share_A, new_share_B, shr_delta);

    }


    // Populate shares of e_i
    void PrecomputeEi() {
      for (std::size_t i = 0; i < mBatchSize; i++) {
	FF shr(1);
	for (std::size_t j = 0; j < mBatchSize; ++j) {
	  if (j == i) continue;
	  shr *= (FF(mID+1) + FF(j)) / (FF(j) - FF(i));
	}
	mSharesOfEi.emplace_back(shr);
      }
    }

    // Maps
    std::map<std::shared_ptr<Gate>, FF> mMapIndShrs;
    std::map<std::shared_ptr<MultBatch>, MultBatchFIPrep> mMapMultBatch;
    std::map<std::shared_ptr<InputBatch>, IOBatchFIPrep> mMapInputBatch;
    std::map<std::shared_ptr<OutputBatch>, IOBatchFIPrep> mMapOutputBatch;

  private:
    // Sizes
    std::size_t mNIndShrs;
    std::size_t mNMultBatches;
    std::size_t mNInOutBatches;

    // Counters
    std::size_t mCTRIndShrs;
    std::size_t mCTRMultBatches;
    std::size_t mCTRInOutBatches;

    std::size_t mBatchSize;


    // Shares of e_i for current party
    std::vector<FF> mSharesOfEi; // len = batchsize
 
   // SHARINGS

    // Individual sharings
    std::vector<FF> mIndShrs;

    // Multiplication batches
    std::vector<MultBatchFIPrep> mMultBatchFIPrep;

    // Input & Output batches
    std::vector<IOBatchFIPrep> mIOBatchFIPrep;


    // NETWORK-RELATED
    std::shared_ptr<scl::Network> mNetwork;
    std::size_t mID;
    std::size_t mParties;

    scl::PRG mPRG;

  };
} // namespace tp

#endif  // CORRELATOR_H
