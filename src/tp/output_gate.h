#ifndef OUT_GATE_H
#define OUT_GATE_H

#include <vector>
#include <assert.h>

#include "tp.h"
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
    void SetNetwork(scl::Network& network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network.Size();
    }

    // First step of the protocol where P1 sends mu to the owner
    void P1SendsMu() {
      if ( mID == 0 ) {
	if ( !mLearned )
	  throw std::invalid_argument("Error: P1 hasn't learned output mu");
	mNetwork.Party(mOwnerID)->Send(mMu);
      }
    }
    
    // The owner receives mu and sets the final value
    void OwnerReceivesMu() {
      if ( mID == mOwnerID ) {
	FF mu;
	mNetwork.Party(0)->Recv(mu);
	mValue = mLambda + mu;
      }
    }

    FF GetValue () { return mValue; }

  private:
    // ID of the party who owns this gate
    std::size_t mOwnerID;

    // Network-related
    scl::Network mNetwork;
    std::size_t mID;
    std::size_t mParties;

    // Protocol-specific
    FF mLambda; // Lambda, learned by owner
    // The value the owner of the gate will obtain as a result
    FF mValue;
  };

  // Used for padding batched outputs
  class PadOutputGate : public OutputGate {
  public:
    PadOutputGate(std::size_t owner_id) : OutputGate(owner_id) {};

    FF GetMu() override { return FF(0); }

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
    // batch to be just 0 shares
    void _DummyPrep() {
      if ( mOutputGatesPtrs.size() != mBatchSize )
	throw std::invalid_argument("The number of output gates does not match the batch size");

      // TODO
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
    void SetNetwork(scl::Network& network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network.Size();
    }

  private:
    // ID of the party who owns this batch
    std::size_t mOwnerID;

    std::size_t mBatchSize;

    // The output gates that are part of this batch
    vec<std::shared_ptr<OutputGate>> mOutputGatesPtrs;

    // The packed sharings associated to this batch
    FF mPackedShrLambda;

    // Network-related
    scl::Network mNetwork;
    std::size_t mID;
    std::size_t mParties;

    // Intermediate-protocol
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
    vec<std::shared_ptr<OutputBatch>> mBatches;
    std::size_t mBatchSize;
  };

} // namespace tp

#endif  // OUT_GATE_H
