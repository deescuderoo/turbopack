#ifndef CORRELATOR_H
#define CORRELATOR_H

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

    // Generates dummy F.I. preprocessing
    void GenIndShrsDummy(FF lambda) {
      mIndShrs = std::vector<FF>(mNIndShrs, lambda);
    }
    void GenMultBatchDummy() {
      mMultBatchFIPrep = std::vector<MultBatchFIPrep>(mNMultBatches, MultBatchFIPrep());      
    }
    void GenIOBatchDummy() {
      mIOBatchFIPrep = std::vector<IOBatchFIPrep>(mNInOutBatches, IOBatchFIPrep());
    }

    void GenPrepDummy(FF lambda) {
      GenIndShrsDummy(lambda);
      GenMultBatchDummy();
      GenIOBatchDummy();
    }

    void GenPrepDummy() {
      GenPrepDummy(FF(0));
    }

    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network->Size();
    }

    void SetThreshold(std::size_t threshold) {
      if ( mParties != threshold + 2*(mBatchSize - 1) + 1 )
	throw std::invalid_argument("It must hold that n = t + 2(k-1) + 1");
      if ( mParties <= 2*threshold )
	throw std::invalid_argument("It must hold that t < n/2");
      mThreshold = threshold;
    }

    // GENERATE F.I. PREPROCESSING

    void GenIndShrsPartiesSend();
    void GenIndShrsPartiesReceive();

    void GenUnpackedShrPartiesSend();
    void GenUnpackedShrPartiesReceive();

    // Zero shares. Used for:
    // Inputs, Outputs, 3xMult
    void GenZeroPartiesSend();
    void GenZeroPartiesReceive();

    // Zero shares for taking a product
    void GenZeroForProdPartiesSend();
    void GenZeroForProdPartiesReceive();

    // Execute the products
    void GenProdPartiesSendP1();
    void GenProdP1ReceivesAndSends();
    void GenProdPartiesReceive();

    // Mapping gates to preprocessed data
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

    // Generate FD Prep from FI Prep
    
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

    // Populate vandermonde matrix
    void PrecomputeVandermonde() {
      mVandermonde.reserve(mParties);
      for (std::size_t i = 0; i < mParties; i++) {
	mVandermonde.emplace_back(std::vector<FF>());
	mVandermonde[i].reserve(mThreshold + 1);
	FF entry(1);
	for (std::size_t j = 0; j < mThreshold + 1; ++j) {
	  mVandermonde[i].emplace_back(entry);
	  entry *= FF(i);
	}
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
    std::vector<std::vector<FF>> mVandermonde; // mParties x (mThreshold + 1)
 

    // SHARINGS

    // Individual sharings
    std::vector<FF> mIndShrs;

    // Multiplication batches
    std::vector<MultBatchFIPrep> mMultBatchFIPrep;

    // Input & Output batches
    std::vector<IOBatchFIPrep> mIOBatchFIPrep;

    // HELPERS TO GET THE F.I. PREP
    std::vector<std::vector<FF>> mUnpackedShrsA; // idx = packed index
    std::vector<std::vector<FF>> mUnpackedShrsB; // idx = packed index
    std::vector<std::vector<FF>> mUnpackedShrsMask; // idx = packed index
    std::vector<std::vector<FF>> mZeroProdShrs; // idx = packed index

    

    // NETWORK-RELATED
    std::shared_ptr<scl::Network> mNetwork;
    std::size_t mID;
    std::size_t mParties;

    std::size_t mThreshold;

    scl::PRG mPRG;
  };

} // namespace tp

#endif  // CORRELATOR_H
