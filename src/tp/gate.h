#ifndef GATE_H
#define GATE_H

#include <vector>
#include <assert.h>

#include "tp.h"

namespace tp {

  class Gate {
  public:

    // Individual share of the mask (lambda...lambda) associated to
    // the output of this gate.
    FF GetShrLambda() { return mIndvShrLambdaC; };

    // Intended to be called by P1
    virtual FF GetMu() = 0;

    // Intended to be called by P1
    bool IsLearned() { return mLearned; };

    // For cleartext evaluation
    bool IsEvaluated() { return mEvaluated; };


    // Get the cleartext value associated to the output of this gate,
    // if computed already (cleartext evaluation)
    virtual FF GetClear() = 0;

    std::shared_ptr<Gate> GetLeft() { return mLeft; }
    std::shared_ptr<Gate> GetRight() { return mRight; }

  protected:
    // Bool that indicates whether P1 learned Mu already
    bool mLearned = false;

    // Sharing of (lambda ... lambda)
    FF mIndvShrLambdaC = FF(0);

    // mu = value - lambda
    // Learned by P1 in the online phase
    FF mMu;

    // Parents
    std::shared_ptr<Gate> mLeft;
    std::shared_ptr<Gate> mRight;

    // For cleartext evaluation
    // Bool that indicates whether the gate has been evaluated
    bool mEvaluated = false;
    // Evaluation of the output wire of the gate
    FF mClear;

    friend class MultBatch;
  };  

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

    void OwnerSendsP1(FF input) {
      mLambda = FF(0); // TODO this should be learned by owner with a
		       // preprocessing protocol
      if (mID == mOwnerID) mNetwork.Party(0)->Send(input - mLambda);
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
  };

  class OutputGate : public Gate {
  public:
    OutputGate(std::shared_ptr<Gate> gate) {
      mLeft = gate;
      mRight = nullptr;
    };

    FF GetMu() {
      if ( !mLearned ) {
	mMu = mLeft->GetMu();
	mLearned = true;
      }
      return mMu;
    };

    FF GetClear() {
      if ( !mEvaluated ) {
	mClear = mLeft->GetClear();
	mEvaluated = true;
      }
      return mClear;
    };

    // TODO output protocol
    
  };


  class AddGate : public Gate {
  public:
    AddGate(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      mLeft = left;
      mRight = right;
      mIndvShrLambdaC = left->GetShrLambda() + right->GetShrLambda();
    };

    FF GetMu() {
      if ( !mLearned ) {
	mMu = mLeft->GetMu() + mRight->GetMu();
	mLearned = true;
      }
      return mMu;
    };

    FF GetClear() {
      if ( !mEvaluated ) {
	mClear = mLeft->GetClear() + mRight->GetClear();
	mEvaluated = true;
      }
      return mClear;
    };

  };

  class MultGate : public Gate {
  public:
    MultGate() {};
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
  class PaddingGate : public MultGate {
  public:
    PaddingGate() : MultGate() {};

    FF GetMu() override { return FF(0); }

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
    // batch to be just 0 shares
    void _DummyPrep() {
      if ( mMultGatesPtrs.size() != mBatchSize )
	throw std::invalid_argument("The number of mult gates does not match the batch size");

      mPackedShrLambdaA = FF(0);
      mPackedShrLambdaB = FF(0);
      mPackedShrLambdaC = FF(0);
      mPackedShrLambdaAB = FF(0);
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
    void P1Sends() {
      if ( mID == 0 ) {

	// 1. P1 assembles mu_A and mu_B

	Vec mu_alpha;
	Vec mu_beta;
	// Here is where the permutation happens!
	for (auto gate : mMultGatesPtrs) {
	  mu_alpha.Emplace(gate->GetLeft()->GetMu());
	  mu_beta.Emplace(gate->GetRight()->GetMu());
	}
      
	// 2. P1 generates shares of mu_A and mu_B
      
	auto poly_A = scl::details::EvPolyFromSecretsAndDegree(mu_alpha, mBatchSize-1, mPRG);
	Vec shares_A = scl::details::SharesFromEvPoly(poly_A, mParties);

	auto poly_B = scl::details::EvPolyFromSecretsAndDegree(mu_beta, mBatchSize-1, mPRG);
	Vec shares_B = scl::details::SharesFromEvPoly(poly_B, mParties);

	// 3. P1 sends the shares

	for (std::size_t i = 0; i < mParties; ++i) {
	  mNetwork.Party(i)->Send(shares_A[i]);
	  mNetwork.Party(i)->Send(shares_B[i]);
	}
      }
    }

    // The parties receive the packed shares of the mu's and store
    // them
    void PartiesReceive() {
      FF shr_mu_A;
      FF shr_mu_B;

      mNetwork.Party(0)->Recv(shr_mu_A);
      mNetwork.Party(0)->Recv(shr_mu_B);

      mPackedShrMuA = shr_mu_A;
      mPackedShrMuB = shr_mu_B;
    }

    // The parties compute locally the necessary Beaver linear
    // combination and send the resulting share back to P1
    void PartiesSend() {
      // Compute share
      FF shr_mu_C;
      shr_mu_C = mPackedShrMuB * mPackedShrLambdaA + mPackedShrMuA * mPackedShrLambdaB + \
	mPackedShrMuA * mPackedShrMuB + mPackedShrLambdaAB - mPackedShrLambdaC;

      // Send to P1
      mNetwork.Party(0)->Send(shr_mu_C); 
    }

    // P1 receives the shares from the parties, reconstructs the mu
    // of the outputs in the batch, and updates these accordingly
    void P1Receives() {
      if (mID == 0) {
	Vec mu_gamma;
	Vec shares;
	for (std::size_t i = 0; i < mParties; ++i) {
	  FF shr_mu_C;
	  mNetwork.Party(i)->Recv(shr_mu_C);
	  shares.Emplace(shr_mu_C);
	}
	mu_gamma = scl::details::SecretsFromSharesAndLength(shares, mBatchSize);

	// P1 updates the mu for the gates in the current batch
	for (std::size_t i = 0; i < mBatchSize; i++) {
	  mMultGatesPtrs[i]->mMu = mu_gamma[i];
	  mMultGatesPtrs[i]->mLearned = true;
	}
      }
    }

  private:
    std::size_t mBatchSize;

    // The mult gates that are part of this batch
    vec<std::shared_ptr<MultGate>> mMultGatesPtrs;

    // The packed sharings associated to this batch
    FF mPackedShrLambdaA;
    FF mPackedShrLambdaB;
    FF mPackedShrLambdaC;
    FF mPackedShrLambdaAB;

    // Network-related
    scl::Network mNetwork;
    std::size_t mID;
    std::size_t mParties;

    // Intermediate-protocol
    scl::PRG mPRG;
    FF mPackedShrMuA;    // Shares of mu_alpha
    FF mPackedShrMuB;    // Shares of mu_beta
    FF mPackedShrMuC;    // Shares of mu_gamma

  };

  // Basically a collection of batches
  class Layer {
  public:
    Layer(std::size_t batch_size) : mBatchSize(batch_size) {
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
      auto padding_gate = std::make_shared<PaddingGate>();
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
    void _DummyPrep() {
      for (auto batch : mBatches) batch->_DummyPrep();
    }

    void ClearEvaluation() {
      for (auto batch : mBatches) batch->GetClear();
    }

    // Intended to be run one after the other
    void P1Sends() { for (auto batch : mBatches) batch->P1Sends(); }
    void PartiesReceive() { for (auto batch : mBatches) batch->PartiesReceive(); }
    void PartiesSend() { for (auto batch : mBatches) batch->PartiesSend(); }
    void P1Receives() { for (auto batch : mBatches) batch->P1Receives(); }

  private:
    vec<std::shared_ptr<MultBatch>> mBatches;
    std::size_t mBatchSize;
  };


  // layer1 = [mbatch1 .... mbatchK], layer2....
  //
  // The preprocessing:
  // for (l in layers):
  //   for batch in layer:
  //     batch.stepONE() // send first round
  // for (l in layers):
  //   for batch in layer:
  //     batch.stepTWO() // recv first round ... etc
  //
  // The online phase (after the input phase):
  // for (l in layers):
  //   for batch in layer:
  //     batch.p1_sends()
  //   for batch in layer:
  //     batch.parties_receive()
  //   for batch in layer:
  //     batch.parties_send()
  //   for batch in layer:
  //     batch.parties_receive()

} // namespace tp

#endif  // GATE_H
