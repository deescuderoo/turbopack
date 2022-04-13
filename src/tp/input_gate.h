#ifndef INP_GATE_H
#define INP_GATE_H

#include <vector>
#include <assert.h>

#include "tp.h"
#include "gate.h"

namespace tp {
  class InputGate : public Gate {
  public:
    InputGate(std::size_t owner_id) : mOwnerID(owner_id) {
      // TODO sample at random, interactively. Get from correlator
      mIndvShrLambdaC = FF(0);
    };

    // Set cleartext inputs, for the case of cleartext evaluation
    void ClearInput(FF input) {
      mClear = input;
      mEvaluated = true;
    };

    // For testing purposes. Sets the input to the given
    // value. Assumes the associated mask is 0, and P1 learns the
    // "masked" value.
    void _SetDummyInput(FF input) {
      mMu = input;
      mLearned = true;
    };

    std::size_t GetOwner() {
      return mOwnerID;
    };

    FF GetMu() {
      if ( !mLearned )
	throw std::invalid_argument("P1 hasn't learned this value yet");
      return mMu;
    };
    
    FF GetClear() {
      if ( !mEvaluated )
	throw std::invalid_argument("This input has not been provided yet");
      return mClear;
    };

    // Protocol-related
    void SetNetwork(scl::Network& network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network.Size();
    }

    void SetInput(FF input) {
      if ( mID == mOwnerID ) mValue = input;
    }

    void OwnerSendsP1() {
      mLambda = FF(0); // TODO this should be learned by owner with a
		       // preprocessing protocol
      if (mID == mOwnerID) mNetwork.Party(0)->Send(mValue - mLambda);
    }

    void P1Receives() {
      if (mID == 0) {
	mNetwork.Party(mOwnerID)->Recv(mMu);
	mLearned = true;
      }
    }

  private:
    // ID of the party who owns this gate
    std::size_t mOwnerID;

    // Network-related
    scl::Network mNetwork;
    std::size_t mID;
    std::size_t mParties;

    // Protocol-specific
    FF mLambda; // Lambda, known by owner
    FF mValue; // Value, known by owner
  };

  // Used for padding batched inputs
  class PadInputGate : public InputGate {
  public:
    PadInputGate(std::size_t owner_id) : InputGate(owner_id) {};

    FF GetMu() override { return FF(0); }

  private:
  };

  class InputBatch {
  public:
    InputBatch(std::size_t owner_id, std::size_t batch_size) : mOwnerID(owner_id), mBatchSize(batch_size) {
      mInputGatesPtrs.reserve(mBatchSize);
    };

    // Adds a new input_gate to the batch. It cannot add more gates than
    // the batch_size
    void Append(std::shared_ptr<InputGate> input_gate) {
      if ( mInputGatesPtrs.size() == mBatchSize )
	throw std::invalid_argument("Trying to batch more than batch_size gates");
      if ( input_gate->GetOwner() != mOwnerID )
	throw std::invalid_argument("Owner IDs do not match");
      mInputGatesPtrs.emplace_back(input_gate); }

    // For testing purposes: sets the required preprocessing for this
    // batch to be just 0 shares
    void _DummyPrep() {
      if ( mInputGatesPtrs.size() != mBatchSize )
	throw std::invalid_argument("The number of input gates does not match the batch size");

      // TODO
    }
                                            
    // For cleartext evaluation: calls GetClear on all its gates to
    // populate their mClear. This could return a vector with these
    // values but we're not needing them
    void GetClear() {
      for (auto gate : mInputGatesPtrs) { gate->GetClear(); }
    }

    // Determines whether the batch is full
    bool HasRoom() { return mInputGatesPtrs.size() < mBatchSize; }
    
    // For fetching input gates
    std::shared_ptr<InputGate> GetInputGate(std::size_t idx) { return mInputGatesPtrs[idx]; }

    // Set network parameters for evaluating the protocol. This is not
    // part of the creation of the batch since sometimes we just want
    // to evaluate in the clear and this won't be needed
    void SetNetwork(scl::Network& network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network.Size();
    }

  private:
    // ID of the party who owns this batch
    std::size_t mOwnerID;

    std::size_t mBatchSize;


    // The input gates that are part of this batch
    vec<std::shared_ptr<InputGate>> mInputGatesPtrs;

    // The packed sharings associated to this batch
    FF mPackedShrLambda;

    // Lambda. Known by owner
    FF mLambda;

    // Network-related
    scl::Network mNetwork;
    std::size_t mID;
    std::size_t mParties;

    // Intermediate-protocol
    scl::PRG mPRG;
  };

  // Basically a collection of batches
  class InputLayer {
  public:
    InputLayer(std::size_t owner_id, std::size_t batch_size) : mOwnerID(owner_id), mBatchSize(batch_size) {
      auto first_batch = std::make_shared<InputBatch>(mOwnerID, mBatchSize);
      // Append a first batch
      mBatches.emplace_back(first_batch);
    };

    // Adds a new input_gate to the layer. It checks if the current
    // batch is full and if so creates a new one.
    void Append(std::shared_ptr<InputGate> input_gate) {
      auto current_batch = mBatches.back(); // accessing last elt
      if ( current_batch->HasRoom() ) {
	current_batch->Append(input_gate);
      } else {
	auto new_batch = std::make_shared<InputBatch>(mOwnerID, mBatchSize);
	new_batch->Append(input_gate);
	mBatches.emplace_back(new_batch);
      }
    }

    std::shared_ptr<InputBatch> GetInputBatch(std::size_t idx) { return mBatches[idx]; }

    // Pads the current batch if necessary
    void Close() {
      auto padding_gate = std::make_shared<PadInputGate>(mOwnerID);
      assert(padding_gate->GetMu() == FF(0));

      auto last_batch = mBatches.back(); // accessing last elt
      while ( last_batch->HasRoom() ) {
	last_batch->Append(padding_gate);
      } 
      // assert(last_batch->HasRoom() == false); // PASSES
    }

    // For testing purposes: sets the required preprocessing for each
    // batch to be just 0 shares
    void _DummyPrep() {
      for (auto batch : mBatches) batch->_DummyPrep();
    }

    void ClearEvaluation() {
      for (auto batch : mBatches) batch->GetClear();
    }

    // // Intended to be run one after the other TODO
    // void P1Sends() { for (auto batch : mBatches) batch->P1Sends(); }
    // void PartiesReceive() { for (auto batch : mBatches) batch->PartiesReceive(); }
    // void PartiesSend() { for (auto batch : mBatches) batch->PartiesSend(); }
    // void P1Receives() { for (auto batch : mBatches) batch->P1Receives(); }

  private:
    std::size_t mOwnerID;
    vec<std::shared_ptr<InputBatch>> mBatches;
    std::size_t mBatchSize;
  };

} // namespace tp

#endif  // INP_GATE_H
