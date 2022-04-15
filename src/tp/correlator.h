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
    // Share of zero
    FF mShrO;

    IOBatchFIPrep() {
      mShrO = FF(0);
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
      mMultBatchFIPrep = std::vector<MultBatchFIPrep>(mNMultBatches, MultBatchFIPrep());
      mIOBatchFIPrep = std::vector<IOBatchFIPrep>(mNInOutBatches, IOBatchFIPrep());
    }

    void PopulateDummy() {
      PopulateDummy(FF(0));
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
    void PrepInputPartiesSendOwner(std::shared_ptr<InputBatch> input_batch);
    
    void PrepInputOwnerReceives(std::shared_ptr<InputBatch> input_batch);


    // PREP OUTPUT BATCH
    void PrepOutputPartiesSendOwner(std::shared_ptr<OutputBatch> output_batch);
    
    void PrepOutputOwnerReceives(std::shared_ptr<OutputBatch> output_batch);

    // PREP MULT BATCH
    void PrepMultPartiesSendP1(std::shared_ptr<MultBatch> mult_batch);
    
    void PrepMultP1ReceivesAndSends();

    void PrepMultPartiesReceive(std::shared_ptr<MultBatch> mult_batch);


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
