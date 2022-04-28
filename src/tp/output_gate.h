#ifndef OUT_GATE_H
#define OUT_GATE_H

#include <vector>
#include <assert.h>

#include "gate.h"

namespace tp {
  class OutputGate : public Gate {
  public:
    OutputGate(std::size_t owner_id) : mOwnerID(owner_id) {};

    OutputGate(std::shared_ptr<Gate> gate) : mOwnerID(0) {
      mLeft = gate;
      mRight = nullptr;
    };

    OutputGate(std::size_t owner_id, std::shared_ptr<Gate> gate) : mOwnerID(owner_id) {
      mLeft = gate;
      mRight = nullptr;
    };

    std::size_t GetOwner() {
      return mOwnerID;
    };

    FF GetMu() {
      if ( !mLearned ) {
	mMu = mLeft->GetMu();
	mLearned = true;
      }
      return mMu;
    }

    FF GetDummyLambda() {
      if ( !mLambdaSet ) {
	mLambda = mLeft->GetDummyLambda();
	mLambdaSet = true;
      }
      return mLambda;
    }    

    void SetLambda(FF lambda) {
      mLambda = lambda;
      mLambdaSet = true;
    }


    FF GetIndvShrLambda() {
      if ( !mIndvShrLambdaCSet ) {
	mIndvShrLambdaC = mLeft->GetIndvShrLambda();
	mIndvShrLambdaCSet = true;
      }
      return mLambda;
    }    

    FF GetDn07Share() {
      if ( !mDn07Set ) {
	mDn07Share = mLeft->GetDn07Share();
	mDn07Set = true;
      }
      return mDn07Share;
    }    

    FF GetClear() {
      if ( !mEvaluated ) {
	mClear = mLeft->GetClear();
	mEvaluated = true;
      }
      return mClear;
    }

    // Set network parameters for evaluating the protocol. This is not
    // part of the creation of the batch since sometimes we just want
    // to evaluate in the clear and this won't be needed
    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network->Size();
    }

    void _DummyPrep(FF lambda) {
      if (mID == mOwnerID) mLambda = lambda;
    }
    void _DummyPrep() {
      _DummyPrep(FF(0));
    }

    // First step of the protocol where P1 sends mu to the owner
    void P1SendsMu() {
      if ( mID == 0 ) {
	mNetwork->Party(mOwnerID)->Send(GetMu());
      }
    }
    
    // The owner receives mu and sets the final value
    void OwnerReceivesMu() {
      if ( mID == mOwnerID ) {
	FF mu;
	mNetwork->Party(0)->Recv(mu);
	mValue = mLambda + mu;
      }
    }

    FF GetValue () { return mValue; }

  private:
    // ID of the party who owns this gate
    std::size_t mOwnerID;

    // Network-related
    std::shared_ptr<scl::Network> mNetwork;
    std::size_t mID;
    std::size_t mParties;

    // Protocol-specific
    FF mLambda; // Lambda, learned by owner
    FF mValue;  // The value the owner will obtain as a result
  };

  // Used for padding batched outputs
  class PadOutputGate : public OutputGate {
  public:
    PadOutputGate(std::size_t owner_id) : OutputGate(owner_id) { mIsPadding = true; }

    FF GetMu() override { return FF(0); }
    FF GetDummyLambda() override { return FF(0); }

    void UpdateParent(std::shared_ptr<Gate> left) {
      mLeft = left;
    }    
  };

  class OutputBatch {
  public:
    OutputBatch(std::size_t owner_id, std::size_t batch_size) : mOwnerID(owner_id), mBatchSize(batch_size) {
      mOutputGatesPtrs.reserve(mBatchSize);
    };

    // Adds a new output_gate to the batch. It cannot add more gates than
    // the batch_size
    void Append(std::shared_ptr<OutputGate> output_gate) {
      if ( mOutputGatesPtrs.size() == mBatchSize )
	throw std::invalid_argument("Trying to batch more than batch_size gates");
      if ( output_gate->GetOwner() != mOwnerID )
	throw std::invalid_argument("Owner IDs do not match");
      mOutputGatesPtrs.emplace_back(output_gate); }

    // For testing purposes: sets the required preprocessing for this
    // batch to be constant shares
    void _DummyPrep(FF lambda) {
      if ( mOutputGatesPtrs.size() != mBatchSize )
	throw std::invalid_argument("The number of output gates does not match the batch size");

      mPackedShrLambda = lambda;
      for (auto input_gate : mOutputGatesPtrs) input_gate->_DummyPrep(lambda);
    }
    void _DummyPrep() {
      _DummyPrep(FF(0));
    }

    std::size_t GetOwner() {
      return mOwnerID;
    };


    // Generates the preprocessing from the lambdas of the inputs
    void PrepFromDummyLambdas() {
      Vec lambda;

      for (std::size_t i = 0; i < mBatchSize; i++) {
	lambda.Emplace(mOutputGatesPtrs[i]->GetDummyLambda());
      }
      // Using deg = BatchSize-1 ensures there's no randomness involved
      auto poly = scl::details::EvPolyFromSecretsAndDegree(lambda, mBatchSize-1, mPRG);
      Vec shares = scl::details::SharesFromEvPoly(poly, mParties);

      mPackedShrLambda = shares[mID];	
    }

    // For cleartext evaluation: calls GetClear on all its gates to
    // populate their mClear.
    void GetClear() {
      for (auto gate : mOutputGatesPtrs) { gate->GetClear(); }
    }

    // Determines whether the batch is full
    bool HasRoom() { return mOutputGatesPtrs.size() < mBatchSize; }
    
    // For fetching output gates
    std::shared_ptr<OutputGate> GetOutputGate(std::size_t idx) { return mOutputGatesPtrs[idx]; }

    // Set network parameters for evaluating the protocol. This is not
    // part of the creation of the batch since sometimes we just want
    // to evaluate in the clear and this won't be needed
    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network->Size();
      for (auto output_gate : mOutputGatesPtrs) output_gate->SetNetwork(network, id);
    }

    void SetPreprocessing(FF packed_shr_lambda) { mPackedShrLambda = packed_shr_lambda; }

    FF GetPackedShrLambda() { return mPackedShrLambda; }

  private:
    // ID of the party who owns this batch
    std::size_t mOwnerID;

    std::size_t mBatchSize;

    // The output gates that are part of this batch
    vec<std::shared_ptr<OutputGate>> mOutputGatesPtrs;

    // The packed sharings associated to this batch
    FF mPackedShrLambda;

    // Network-related
    std::shared_ptr<scl::Network> mNetwork;
    std::size_t mID;
    std::size_t mParties;

    scl::PRG mPRG;
  };

  // Basically a collection of batches
  class OutputLayer {
  public:
    OutputLayer(std::size_t owner_id, std::size_t batch_size) : mOwnerID(owner_id), mBatchSize(batch_size) {
      auto first_batch = std::make_shared<OutputBatch>(mOwnerID, mBatchSize);
      // Append a first batch
      mBatches.emplace_back(first_batch);
    };

    // Adds a new output_gate to the layer. It checks if the current
    // batch is full and if so creates a new one.
    void Append(std::shared_ptr<OutputGate> output_gate) {
      auto current_batch = mBatches.back(); // accessing last elt
      if ( current_batch->HasRoom() ) {
	current_batch->Append(output_gate);
      } else {
	auto new_batch = std::make_shared<OutputBatch>(mOwnerID, mBatchSize);
	new_batch->Append(output_gate);
	mBatches.emplace_back(new_batch);
      }
    }

    std::shared_ptr<OutputBatch> GetOutputBatch(std::size_t idx) { return mBatches[idx]; }

    // Pads the current batch if necessary
    void Close() {
      auto padding_gate = std::make_shared<PadOutputGate>(mOwnerID);
      padding_gate->UpdateParent(padding_gate);
      assert(padding_gate->GetMu() == FF(0));

      auto last_batch = mBatches.back(); // accessing last elt
      while ( last_batch->HasRoom() ) {
	last_batch->Append(padding_gate);
      } 
      // assert(last_batch->HasRoom() == false); // PASSES
    }

    // For testing purposes: sets the required preprocessing for each
    // batch to be just 0 shares
    void _DummyPrep(FF lambda) {
      for (auto batch : mBatches) batch->_DummyPrep(lambda);
    }
    void _DummyPrep() {
      for (auto batch : mBatches) batch->_DummyPrep();
    }
    void PrepFromDummyLambdas() {
      for (auto batch : mBatches) batch->PrepFromDummyLambdas();
    }

    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network->Size();
      for (auto batch : mBatches) batch->SetNetwork(network, id);
    }

    void ClearEvaluation() {
      for (auto batch : mBatches) batch->GetClear();
    }

    // Metrics
    std::size_t GetSize() { return mBatches.size(); }

  private:
    std::size_t mOwnerID;
    vec<std::shared_ptr<OutputBatch>> mBatches;
    std::size_t mBatchSize;

    // Network-related
    std::shared_ptr<scl::Network> mNetwork;
    std::size_t mID;
    std::size_t mParties;

    friend class Circuit;
  };

} // namespace tp

#endif  // OUT_GATE_H
