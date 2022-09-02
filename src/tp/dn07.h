#ifndef DN07_H
#define DN07_H

#include <iostream>
#include <map>
#include <assert.h>
#include <queue>

#include "tp.h"
#include "mult_gate.h"
#include "input_gate.h"
#include "output_gate.h"
#include "circuits.h"

namespace tp {
  struct RandShr {
    FF shr;
    
    RandShr(FF share) {
      shr = share;
    }
    RandShr() {
      shr = FF(0);
    }
  };

  struct DoubleShr {
    FF shr;
    FF dshr;
    
    DoubleShr(FF share, FF share_d) {
      shr = share;
      dshr = share_d;
    }
    DoubleShr(FF share) {
      shr = share;
      dshr = share;
    }
    DoubleShr() {
      shr = FF(0);
      dshr = FF(0);
    }
  };

  class DN07 {
  public:
    DN07(std::size_t parties, std::size_t threshold) : mCTRDoubleShrs(0), mCTRRandShrs(0), \
						       mParties(parties), mThreshold(threshold) {
      if ( mParties <= 2*mThreshold )
	throw std::invalid_argument("It must hold that t < n/2");
      PrecomputeVandermonde();
    }

    void SetCircuit(Circuit& circuit);

    Circuit GetCircuit() { return mCircuit; }

    void DummyPrep(FF prep);

    void DummyPrep();

    void SetInputs(std::vector<FF> inputs);

    // OFFLINE PHASE
    void PrepPartiesSend();
    void PrepPartiesReceive();

    // FD OFFLINE
    void FDMapPrepToGates();
    void FDMultPartiesSendP1();
    void FDMultP1Receives();
    
    // ONLINE PHASE
    void InputPartiesSendOwners();
    void InputOwnersReceiveAndSendParties();
    void InputPartiesReceive();

    void MultPartiesSendP1(std::size_t layer);
    void MultP1Receives(std::size_t layer);
    void MultP1SendsParties(std::size_t layer);
    void MultP1ReceivesAndSendsParties(std::size_t layer) {
      MultP1Receives(layer);
      MultP1SendsParties(layer);
    }
    void MultPartiesReceive(std::size_t layer);

    void OutputPartiesSendOwners();
    
    void OutputOwnersReceive();

    FF GetOutput(std::size_t owner_id, std::size_t idx) {
      return mMapResults[ mCircuit.mFlatOutputGates[owner_id][idx] ];
    }

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

  private:
    Circuit mCircuit;

    // Amounts
    std::size_t mNMults;
    std::size_t mNInputs;

    // Counters
    std::size_t mCTRDoubleShrs;
    std::size_t mCTRRandShrs;

    // Preprocessing containers
    std::vector<DoubleShr> mDShrs;
    std::vector<RandShr> mRShrs;

    // Maps (prep: map to indexes to the above containers)
    std::map<std::shared_ptr<MultGate>, std::size_t> mMapMults;
    std::map<std::shared_ptr<InputGate>, std::size_t> mMapInputs;

    // Map with results
    std::map<std::shared_ptr<OutputGate>, FF> mMapResults;

    std::size_t mParties;
    std::size_t mThreshold;

    scl::PRG mPRG;
    std::vector<std::vector<FF>> mVandermonde; // mParties x (mThreshold + 1)

    std::queue<Vec> mP1SharesToSend; 
    std::queue<Vec> mMapP1SharesFromLast; 
  };

} // namespace tp

#endif  // DN07_H
