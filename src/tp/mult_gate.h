#ifndef MULT_GATE_H
#define MULT_GATE_H

#include <vector>
#include <assert.h>

#include "gate.h"

namespace tp {
  class MultGate : public Gate {
  public:
    MultGate() {}; // for the padding gates below
    MultGate(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      mLeft = left;
      mRight = right;
    };

    FF GetMu() {
      if ( !mLearned ) {
	throw std::invalid_argument("Run multiplication protocol first");
      }
      return mMu;
    };

    void SetDummyLambda(FF lambda) {
      mLambda = lambda;
      mLambdaSet = true;
    };

    FF GetDummyLambda() {
      if ( !mLambdaSet ) {
	throw std::invalid_argument("Lambda is not set in this multiplication gate");
      }
      return mLambda;
    };

    void SetIndvShrLambda(FF indv_shr) {
      mIndvShrLambdaC = indv_shr;
      mIndvShrLambdaCSet = true;
    }

    FF GetIndvShrLambda() {
      if ( !mIndvShrLambdaCSet )
	throw std::invalid_argument("IndvShrLambda is not set in this multiplication gate");
      return mIndvShrLambdaC;
    }

    FF GetDn07Share() {
      if ( !mDn07Set )
	throw std::invalid_argument("Dn07 shares is not set in this multiplication gate");
      return mDn07Share;
    }

    FF GetClear() {
      if ( !mEvaluated ) {
	mClear = mLeft->GetClear() * mRight->GetClear();
	mEvaluated = true;
      }
      return mClear;
    }

  private:
  };

  // Used for padding batched multiplications
  class PadMultGate : public MultGate {
  public:
    PadMultGate() : MultGate() { mIsPadding = true; };

    FF GetMu() override { return FF(0); }
    FF GetDummyLambda() override { return FF(0); }

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
    void _DummyPrep(FF lambda_A, FF lambda_B, FF lambda_C) {
      if ( mMultGatesPtrs.size() != mBatchSize )
	throw std::invalid_argument("The number of mult gates does not match the batch size");

      mPackedShrLambdaA = lambda_A;
      mPackedShrLambdaB = lambda_B;
      mPackedShrDeltaC = lambda_A * lambda_B - lambda_C;
    };

    void _DummyPrep() {
      _DummyPrep(FF(0), FF(0), FF(0));
    };
    
    // Generates the preprocessing from the lambdas of the inputs
    void PrepFromDummyLambdas() {
      Vec lambda_A;
      Vec lambda_B;
      Vec delta_C;

      for (std::size_t i = 0; i < mBatchSize; i++) {
	auto l_A = mMultGatesPtrs[i]->GetLeft()->GetDummyLambda();
	auto l_B = mMultGatesPtrs[i]->GetRight()->GetDummyLambda();
	auto l_C = mMultGatesPtrs[i]->GetDummyLambda();
	auto d_C = l_A * l_B - l_C;

	lambda_A.Emplace(l_A);
	lambda_B.Emplace(l_B);
	delta_C.Emplace(d_C);
      }
      
      // Using deg = BatchSize-1 ensures there's no randomness involved
      auto poly_A = scl::details::EvPolyFromSecretsAndDegree(lambda_A, mBatchSize-1, mPRG);
      mPackedShrLambdaA = poly_A.Evaluate(FF(mID));
	
      auto poly_B = scl::details::EvPolyFromSecretsAndDegree(lambda_B, mBatchSize-1, mPRG);
      mPackedShrLambdaB = poly_B.Evaluate(FF(mID));

      auto poly_C = scl::details::EvPolyFromSecretsAndDegree(delta_C, mBatchSize-1, mPRG);
      mPackedShrDeltaC = poly_C.Evaluate(FF(mID));
    }

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
    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network->Size();
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

    void RunProtocol() {
      P1Sends();
      PartiesReceive();
      PartiesSend();
      P1Receives();
    }

    void SetPreprocessing(FF shr_lambda_A, FF shr_lambda_B, FF shr_delta_C) {
      mPackedShrLambdaA = shr_lambda_A;
      mPackedShrLambdaB = shr_lambda_B;
      mPackedShrDeltaC = shr_delta_C;
    }


  private:
    std::size_t mBatchSize;

    // The mult gates that are part of this batch
    vec<std::shared_ptr<MultGate>> mMultGatesPtrs;

    // The packed sharings associated to this batch
    FF mPackedShrLambdaA;
    FF mPackedShrLambdaB;
    FF mPackedShrDeltaC;

    // Network-related
    std::shared_ptr<scl::Network> mNetwork;
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
    void _DummyPrep(FF lambda_A, FF lambda_B, FF lambda_C) {
      for (auto batch : mBatches) batch->_DummyPrep(lambda_A, lambda_B, lambda_C);
    }
    void _DummyPrep() {
      for (auto batch : mBatches) batch->_DummyPrep();
    }
    void PrepFromDummyLambdas() {
      for (auto batch : mBatches) batch->PrepFromDummyLambdas();
    }

    void ClearEvaluation() {
      for (auto batch : mBatches) batch->GetClear();
    }

    // Set network parameters for evaluating the protocol. This is not
    // part of the creation of the layer since sometimes we just want
    // to evaluate in the clear and this won't be needed
    // To be used after the layer is closed
    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network->Size();
      for (auto batch : mBatches) batch->SetNetwork(network, id);
    }

    // For testing purposes, to avoid blocking
    void P1Sends() { for (auto batch : mBatches) batch->P1Sends(); }
    void PartiesReceive() { for (auto batch : mBatches) batch->PartiesReceive(); }
    void PartiesSend() { for (auto batch : mBatches) batch->PartiesSend(); }
    void P1Receives() { for (auto batch : mBatches) batch->P1Receives(); }
    
    // Metrics
    std::size_t GetSize() { return mBatches.size(); }

  private:
    vec<std::shared_ptr<MultBatch>> mBatches;
    std::size_t mBatchSize;

    // Network-related
    std::shared_ptr<scl::Network> mNetwork;
    std::size_t mID;
    std::size_t mParties;

    friend class Circuit;
  };
} // namespace tp

#endif  // MULT_GATE_H
