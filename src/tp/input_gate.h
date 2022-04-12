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

  // // Used for padding batched inputs
  // class PadInputGate : public InputGate {
  // public:
  //   PadMultGate() : MultGate() {};

  //   FF GetMu() override { return FF(0); }

  //   void UpdateParents(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
  //     mLeft = left;
  //     mRight = right;
  //   }

  // private:
  // };
} // namespace tp

#endif  // INP_GATE_H
