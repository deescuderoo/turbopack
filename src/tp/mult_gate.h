#ifndef MULT_GATE_H
#define MULT_GATE_H

#include <vector>
#include <assert.h>

#include "tp.h"
#include "gate.h"

namespace tp {
  class MultGate : public Gate {
  public:
    MultGate() {}; // for the padding gates below
    MultGate(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      mLeft = left;
      mRight = right;
      // TODO sample at random, interactively. Get from correlator
      mIndvShrLambdaC = FF(0);
    };

    FF GetMu() {
      if ( !mLearned ) {
	throw std::invalid_argument("Run multiplication protocol first");
      }
      return mMu;
    };

    FF GetClear() {
      if ( !mEvaluated ) {
	mClear = mLeft->GetClear() * mRight->GetClear();
	mEvaluated = true;
      }
      return mClear;
    };
  private:
  };

  // Used for padding batched multiplications
  class PadMultGate : public MultGate {
  public:
    PadMultGate() : MultGate() {};

    FF GetMu() override { return FF(0); }

    // Just a technicality needed to make the parents of this gate be
    // itself when instantiated
    void UpdateParents(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      mLeft = left;
      mRight = right;
    }

  private:
  };
    
  class MultBatch {
  public:
    MultBatch(std::size_t batch_size) : mBatchSize(batch_size) {
      mMultGatesPtrs.reserve(mBatchSize);
    };

    // Adds a new mult_gate to the batch. It cannot add more gates than
    // the batch_size
    void Append(std::shared_ptr<MultGate> mult_gate) {
      if ( mMultGatesPtrs.size() == mBatchSize )
	throw std::invalid_argument("Trying to batch more than batch_size gates");
      mMultGatesPtrs.emplace_back(mult_gate); };

    // For testing purposes: sets the required preprocessing for this
    // batch to be just constant shares
    void _DummyPrep(FF lambda_A, FF lambda_B, FF delta_C) {
      if ( mMultGatesPtrs.size() != mBatchSize )
	throw std::invalid_argument("The number of mult gates does not match the batch size");

      mPackedShrLambdaA = lambda_A;
      mPackedShrLambdaB = lambda_B;
      mPackedShrDeltaC = delta_C;
    };

    void _DummyPrep() {
      _DummyPrep(FF(0), FF(0), FF(0));
    };
    
    // For cleartext evaluation: calls GetClear on all its gates to
    // populate their mClear. This could return a vector with these
    // values but we're not needing them
    void GetClear() {
      for (auto gate : mMultGatesPtrs) { gate->GetClear(); }
    }

    // Determines whether the batch is full
    bool HasRoom() { return mMultGatesPtrs.size() < mBatchSize; }
    
    // For fetching mult gates
    std::shared_ptr<MultGate> GetMultGate(std::size_t idx) { return mMultGatesPtrs[idx]; }

    // Set network parameters for evaluating the protocol. This is not
    // part of the creation of the batch since sometimes we just want
    // to evaluate in the clear and this won't be needed
    void SetNetwork(scl::Network& network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network.Size();
    }

    // First step of the protocol where P1 sends the packed shares of
    // the mu of the inputs
    void P1Sends();

    // The parties receive the packed shares of the mu's and store
    // them
    void PartiesReceive();

    // The parties compute locally the necessary Beaver linear
    // combination and send the resulting share back to P1
    void PartiesSend();

    // P1 receives the shares from the parties, reconstructs the mu
    // of the outputs in the batch, and updates these accordingly
    void P1Receives();

  private:
    std::size_t mBatchSize;

    // The mult gates that are part of this batch
    vec<std::shared_ptr<MultGate>> mMultGatesPtrs;

    // The packed sharings associated to this batch
    FF mPackedShrLambdaA;
    FF mPackedShrLambdaB;
    FF mPackedShrDeltaC;

    // Network-related
    scl::Network mNetwork;
    std::size_t mID;
    std::size_t mParties;

    // Intermediate-protocol
    scl::PRG mPRG;
    FF mPackedShrMuA;    // Shares of mu_alpha
    FF mPackedShrMuB;    // Shares of mu_beta
  };

  // Basically a collection of batches
  class MultLayer {
  public:
    MultLayer(std::size_t batch_size) : mBatchSize(batch_size) {
      auto first_batch = std::make_shared<MultBatch>(mBatchSize);
      // Append a first batch
      mBatches.emplace_back(first_batch);
    };

    // Adds a new mult_gate to the layer. It checks if the current
    // batch is full and if so creates a new one.
    void Append(std::shared_ptr<MultGate> mult_gate) {
      auto current_batch = mBatches.back(); // accessing last elt
      if ( current_batch->HasRoom() ) {
	current_batch->Append(mult_gate);
      } else {
	auto new_batch = std::make_shared<MultBatch>(mBatchSize);
	new_batch->Append(mult_gate);
	mBatches.emplace_back(new_batch);
      }
    }

    std::shared_ptr<MultBatch> GetMultBatch(std::size_t idx) { return mBatches[idx]; }

    // Pads the current batch if necessary
    void Close() {
      auto padding_gate = std::make_shared<PadMultGate>();
      padding_gate->UpdateParents(padding_gate, padding_gate);

      assert(padding_gate->GetMu() == FF(0));
      assert(padding_gate->GetLeft()->GetMu() == FF(0));
      assert(padding_gate->GetRight()->GetMu() == FF(0));      

      auto last_batch = mBatches.back(); // accessing last elt
      while ( last_batch->HasRoom() ) {
	last_batch->Append(padding_gate);
      } 
      // assert(last_batch->HasRoom() == false); // PASSES
    }

    // For testing purposes: sets the required preprocessing for each
    // batch to be just 0 shares
    void _DummyPrep(FF lambda_A, FF lambda_B, FF delta_C) {
      for (auto batch : mBatches) batch->_DummyPrep(lambda_A, lambda_B, delta_C);
    }
    void _DummyPrep() {
      for (auto batch : mBatches) batch->_DummyPrep();
    }

    void ClearEvaluation() {
      for (auto batch : mBatches) batch->GetClear();
    }

    // Set network parameters for evaluating the protocol. This is not
    // part of the creation of the layer since sometimes we just want
    // to evaluate in the clear and this won't be needed
    // To be used after the layer is closed
    void SetNetwork(scl::Network& network, std::size_t id) {
      for (auto batch : mBatches) batch->SetNetwork(network, id);
    }

    void RunProtocol() {
      for (auto batch : mBatches) batch->P1Sends();
      for (auto batch : mBatches) batch->PartiesReceive(); 
      for (auto batch : mBatches) batch->PartiesSend(); 
      for (auto batch : mBatches) batch->P1Receives(); 
    }

    // For testing purposes, to avoid blocking
    void _P1Sends() { for (auto batch : mBatches) batch->P1Sends(); }
    void _PartiesReceive() { for (auto batch : mBatches) batch->PartiesReceive(); }
    void _PartiesSend() { for (auto batch : mBatches) batch->PartiesSend(); }
    void _P1Receives() { for (auto batch : mBatches) batch->P1Receives(); }

  private:
    vec<std::shared_ptr<MultBatch>> mBatches;
    std::size_t mBatchSize;
  };
} // namespace tp

#endif  // MULT_GATE_H
