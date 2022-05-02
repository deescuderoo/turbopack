#include "tp/dn07.h"

namespace tp {
  void DN07::SetCircuit(Circuit& circuit) {
    assert( circuit.mParties == mParties );

    mCircuit = circuit;
    mNMults = circuit.GetSize();
    mNInputs = circuit.GetNInputs();

    mDShrs.reserve(mNMults);
    mRShrs.reserve(mNInputs);
  }

  void DN07::DummyPrep(FF prep) {
    mDShrs = std::vector<DoubleShr>(mNMults, DoubleShr(prep));
    mRShrs = std::vector<RandShr>(mNInputs, RandShr(prep));
  }

  void DN07::DummyPrep() {
    mDShrs = std::vector<DoubleShr>(mNMults);
    mRShrs = std::vector<RandShr>(mNInputs);
  }

  void DN07::SetInputs(std::vector<FF> inputs) {
    if ( inputs.size() != mCircuit.mFlatInputGates[mCircuit.mID].size() )
      throw std::invalid_argument("Number of inputs do not match");
    // Set input and send to P1
    for (std::size_t i = 0; i < inputs.size(); i++) {
      mCircuit.mFlatInputGates[mCircuit.mID][i]->SetInput(inputs[i]); // sets the mValue member of the gate
    }      
  }

  // OFFLINE PHASE
  void DN07::PrepPartiesSend() {
    std::size_t degree = mThreshold;
    std::size_t n_amount = mNInputs;
    std::size_t n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);
    for ( std::size_t block = 0; block < n_blocks; block++ ) {
      // 1 sample secret and shares
      FF secret = FF::Random(mPRG);

      auto poly = scl::details::EvPolyFromSecretAndDegree(secret, degree, mPRG);
      auto shares = scl::details::SharesFromEvPoly(poly, mParties);


      // 2 send shares
      for ( std::size_t party = 0; party < mParties; party++ ){
	mCircuit.mNetwork->Party(party)->Send(shares[party]);
      }
    }

    // Double shares
    n_amount = mNMults;
    n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);
    for ( std::size_t block = 0; block < n_blocks; block++ ) {
      // 1 sample secret and shares
      FF secret = FF::Random(mPRG);

      auto poly = scl::details::EvPolyFromSecretAndDegree(secret, degree, mPRG);
      auto poly_d = scl::details::EvPolyFromSecretAndDegree(secret, 2*degree, mPRG);
      auto shares = scl::details::SharesFromEvPoly(poly, mParties);
      auto shares_d = scl::details::SharesFromEvPoly(poly_d, mParties);
	
      // 2 send shares
      for ( std::size_t party = 0; party < mParties; party++ ){
	mCircuit.mNetwork->Party(party)->Send(shares[party]);
	mCircuit.mNetwork->Party(party)->Send(shares_d[party]);
      }
    }
  }

  void DN07::PrepPartiesReceive() {
    std::size_t n_amount = mNInputs;
    std::size_t n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);
    for ( std::size_t block = 0; block < n_blocks; block++ ) {
      // 1 receive shares
      Vec recv_shares;
      recv_shares.Reserve(mParties);
      for (std::size_t party = 0; party < mParties; party++) {
	FF buffer;
	mCircuit.mNetwork->Party(party)->Recv(buffer);
	recv_shares.Emplace(buffer);
      }
      // 2 multiply by Vandermonde
      for ( std::size_t shr_idx = 0; shr_idx < mThreshold+1; shr_idx++ ){
	FF shr(0);
	for ( std::size_t j = 0; j < mParties; j++ ){
	  shr += mVandermonde[j][shr_idx] * recv_shares[j];
	}
	mRShrs.emplace_back(RandShr(shr));
      }
    }

    n_amount = mNMults;
    n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);
    for ( std::size_t block = 0; block < n_blocks; block++ ) {
      // 1 receive shares
      Vec recv_shares;
      Vec recv_shares_d;
      recv_shares.Reserve(mParties);
      recv_shares_d.Reserve(mParties);
      for (std::size_t parties = 0; parties < mParties; parties++) {
	FF buffer;
	mCircuit.mNetwork->Party(parties)->Recv(buffer);
	recv_shares.Emplace(buffer);
	mCircuit.mNetwork->Party(parties)->Recv(buffer);
	recv_shares_d.Emplace(buffer);
      }

      // 2 multiply by Vandermonde
      for ( std::size_t shr_idx = 0; shr_idx < mThreshold+1; shr_idx++ ){
	FF shr(0);
	FF shr_d(0);
	for ( std::size_t j = 0; j < mParties; j++ ){
	  shr += mVandermonde[j][shr_idx] * recv_shares[j];
	  shr_d += mVandermonde[j][shr_idx] * recv_shares_d[j];
	}
	mDShrs.emplace_back(DoubleShr(shr, shr_d));
      }
    }
  }



  // ONLINE PHASE

  void DN07::InputPartiesSendOwners() {
    for (std::size_t owner_id = 0; owner_id < mCircuit.mClients; owner_id++) {
      for (auto input_gate : mCircuit.mFlatInputGates[owner_id]) {
	if ( mCircuit.mID < mThreshold + 1 ) { // Only P1 .. Pt+1 are needed
	  mCircuit.mNetwork->Party(owner_id)->Send(mRShrs[mCTRRandShrs].shr);
	}
	// Update map
	mMapInputs[input_gate] = mCTRRandShrs++;
      }
    }
  }
    
  void DN07::InputOwnersReceiveAndSendParties() {
    // input owner receives
    for (auto input_gate : mCircuit.mFlatInputGates[mCircuit.mID]) {
      if (mCircuit.mID == input_gate->GetOwner()) {	
	Vec recv_shares;
	recv_shares.Reserve(mThreshold+1);
	for ( std::size_t i = 0; i < mThreshold+1; i++ ) {
	  FF share;
	  mCircuit.mNetwork->Party(i)->Recv(share);
	  recv_shares.Emplace(share);
	}
	FF mask = scl::details::SecretFromShares(recv_shares);

	// Input owner masks and sends
	FF input = input_gate->GetValue();
	FF masked = input - mask;
	for ( std::size_t i = 0; i < mParties; i++ ) {
	  mCircuit.mNetwork->Party(i)->Send(masked);
	}
      }
    }
  }

  void DN07::InputPartiesReceive() {
    for (std::size_t owner_id = 0; owner_id < mCircuit.mClients; owner_id++) {
      for (auto input_gate : mCircuit.mFlatInputGates[owner_id]) {
	// Parties receive
	FF masked;
	mCircuit.mNetwork->Party(owner_id)->Recv(masked);

	// Compute share
	FF share = masked + mRShrs[ mMapInputs[input_gate] ].shr;

	// Update map of shares
	input_gate->SetDn07Share(share);
      }
    }
  }

  void DN07::MultPartiesSendP1(std::size_t layer) {
    for (auto mult_gate : mCircuit.mFlatMultLayers[layer]) {
      // Send masked shares to P1
      FF shr = mult_gate->GetLeft()->GetDn07Share() * mult_gate->GetRight()->GetDn07Share() \
	- mDShrs[mCTRDoubleShrs].dshr;

      mCircuit.mNetwork->Party(0)->Send(shr);

      // Update map
      mMapMults[mult_gate] = mCTRDoubleShrs++;
    }
  }

  void DN07::MultP1Receives(std::size_t layer) {
    if (mCircuit.mID == 0) {
      for (auto mult_gate : mCircuit.mFlatMultLayers[layer]) {
	Vec shares;
	for (std::size_t i = 0; i < mParties; ++i) {
	  FF shr;
	  mCircuit.mNetwork->Party(i)->Recv(shr);
	  shares.Emplace(shr);
	}
	FF secret = scl::details::SecretFromShares(shares);

	// P1 reconstructs and sends
	Vec y_points;
	y_points.Reserve(mThreshold+1);
	y_points.Emplace(secret);
	for (std::size_t i = 1; i < mThreshold+1; ++i) y_points.Emplace(FF(0));

	Vec x_points;
	x_points.Reserve(mThreshold+1);

	for (std::size_t i = 0; i < mThreshold+1; ++i) x_points.Emplace(FF(i));

	auto poly = scl::details::EvPolynomial<FF>(x_points, y_points);
	auto shares_to_send = scl::details::SharesFromEvPoly(poly, mParties);
	mP1SharesToSend.push(shares_to_send);
      }
    }
  }
  void DN07::MultP1SendsParties(std::size_t layer) {
    if (mCircuit.mID == 0) {
      for (auto mult_gate : mCircuit.mFlatMultLayers[layer]) {
	for ( std::size_t i = mThreshold; i < mCircuit.mParties; i++ ) {
	  mCircuit.mNetwork->Party(i)->Send(mP1SharesToSend.front()[i]);
	}
	mP1SharesToSend.pop();
      }
    }
  }

  void DN07::MultPartiesReceive(std::size_t layer) {
    for (auto mult_gate : mCircuit.mFlatMultLayers[layer]) {
      if (mCircuit.mID >= mThreshold) {
	FF masked;
	mCircuit.mNetwork->Party(0)->Recv(masked);
	
	// Compute share
	FF share = masked + mDShrs[ mMapMults[mult_gate] ].shr;

	// Update map of shares
	mult_gate->SetDn07Share(share);
      } else {
	FF share = mDShrs[ mMapMults[mult_gate] ].shr;
	mult_gate->SetDn07Share(share);
      }
    }
  }

  void DN07::OutputPartiesSendOwners() {
    if ( mCircuit.mID < mThreshold + 1 ) { // Only P1 .. Pt+1 are needed
      for (std::size_t owner_id = 0; owner_id < mCircuit.mClients; owner_id++) {
	for (auto output_gate : mCircuit.mFlatOutputGates[owner_id]) {
	  FF share = output_gate->GetDn07Share();
	  mCircuit.mNetwork->Party(owner_id)->Send(share);
	}
      }
    }
  }
    
  void DN07::OutputOwnersReceive() {
    // Owner receives
    for (auto output_gate : mCircuit.mFlatOutputGates[mCircuit.mID]) {
      if (mCircuit.mID == output_gate->GetOwner()) {
	Vec recv_shares;
	recv_shares.Reserve(mThreshold+1);
	for ( std::size_t i = 0; i < mThreshold+1; i++ ) {
	  FF share;
	  mCircuit.mNetwork->Party(i)->Recv(share);
	  recv_shares.Emplace(share);
	}
	FF result = scl::details::SecretFromShares(recv_shares);

	mMapResults[output_gate] = result;
      }
    }
  }


}

