#include "tp/correlator.h"

namespace tp {
  // GEN F.I. PREP
  void Correlator::GenIndShrsPartiesSend() {
    std::size_t degree = mParties - mBatchSize;
    std::size_t n_blocks = (mNIndShrs + (mThreshold + 1) -1) / (mThreshold + 1);
    for ( std::size_t block = 0; block < n_blocks; block++ ) {
      // 1 sample secret and shares
      FF secret = FF::Random(mPRG);
      
      Vec secrets(std::vector<FF>(mBatchSize, secret));

      auto poly = scl::details::EvPolyFromSecretsAndDegree(secrets, degree, mPRG);
      auto shares = scl::details::SharesFromEvPoly(poly, mParties);

      // 2 send shares
      for ( std::size_t party = 0; party < mParties; party++ ){
        mNetwork->Party(party)->Send(shares[party]);
      }
    }
  }

  void Correlator::GenIndShrsPartiesReceive() {
    assert(mIndShrs.size() == 0);
    std::size_t n_blocks = (mNIndShrs + (mThreshold + 1) -1) / (mThreshold + 1);
    for ( std::size_t block = 0; block < n_blocks; block++ ) {
      // 1 receive shares
      std::vector<FF> recv_shares;
      recv_shares.reserve(mParties);
      for (std::size_t parties = 0; parties < mParties; parties++) {
	FF buffer;
        mNetwork->Party(parties)->Recv(buffer);
	recv_shares.emplace_back(buffer);
      }
      // 2 multiply by Vandermonde
      for ( std::size_t shr_idx = 0; shr_idx < mThreshold+1; shr_idx++ ){
	FF shr(0);
	for ( std::size_t j = 0; j < mParties; j++ ){
	  shr += mVandermonde[j][shr_idx] * recv_shares[j];
	}
	mIndShrs.emplace_back(shr);
      }
    }
  }

  void Correlator::GenUnpackedShrPartiesSend() {
    std::size_t degree = mThreshold;
    std::size_t n_amount = 3*mNMultBatches; // 2 for the two factors, 1 for the multiplication
    std::size_t n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);

    for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
      for ( std::size_t block = 0; block < n_blocks; block++ ) {
	// 1 sample secret and shares
	FF secret = FF::Random(mPRG);
	// FF secret(0);

	auto poly = scl::details::EvPolyFromSecretAndPointAndDegree(secret, FF(-pack_idx), degree, mPRG);
	auto shares = scl::details::SharesFromEvPoly(poly, mParties);
	
	// 2 send shares
	for ( std::size_t party = 0; party < mParties; party++ ){
	  mNetwork->Party(party)->Send(shares[party]);
	}
      }
    }

  }
  void Correlator::GenUnpackedShrPartiesReceive() {
    std::size_t n_amount = 3*mNMultBatches;
    std::size_t n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);
    mUnpackedShrsA.reserve(mBatchSize);
    mUnpackedShrsB.reserve(mBatchSize);
    mUnpackedShrsMask.reserve(mBatchSize);

    for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
      std::size_t ctr(0);
      mUnpackedShrsA.emplace_back(std::vector<FF>());
      mUnpackedShrsA[pack_idx].reserve(mNMultBatches);
      mUnpackedShrsB.emplace_back(std::vector<FF>());
      mUnpackedShrsB[pack_idx].reserve(mNMultBatches);
      mUnpackedShrsMask.emplace_back(std::vector<FF>());
      mUnpackedShrsMask[pack_idx].reserve(mNMultBatches);

      for ( std::size_t block = 0; block < n_blocks; block++ ) {
	// 1 receive shares
	Vec recv_shares;
	recv_shares.Reserve(mParties);
	for (std::size_t parties = 0; parties < mParties; parties++) {
	  FF buffer;
	  mNetwork->Party(parties)->Recv(buffer);
	  recv_shares.Emplace(buffer);
	}
	// 2 multiply by Vandermonde
	for ( std::size_t shr_idx = 0; shr_idx < mThreshold+1; shr_idx++ ){
	  FF shr(0);
	  for ( std::size_t j = 0; j < mParties; j++ ){
	    shr += mVandermonde[j][shr_idx] * recv_shares[j];
	  }
	  if (ctr < mNMultBatches) mUnpackedShrsA[pack_idx].emplace_back(shr);
	  if ( (mNMultBatches <= ctr) && (ctr < 2*mNMultBatches) ) mUnpackedShrsB[pack_idx].emplace_back(shr);
	  if ( (2*mNMultBatches <= ctr) && (ctr < 3*mNMultBatches) ) mUnpackedShrsMask[pack_idx].emplace_back(shr);
	  ctr++;
	}
      }
    }
    // Create Mult-related data
    for ( std::size_t i = 0; i < mNMultBatches; i++ ) {
      MultBatchFIPrep data(FF(0));
      for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
	data.mShrA += mSharesOfEi[pack_idx] * mUnpackedShrsA[pack_idx][i];
	data.mShrB += mSharesOfEi[pack_idx] * mUnpackedShrsB[pack_idx][i];
      }
      mMultBatchFIPrep.emplace_back(data);
    }
  }

  // Zero shares. Used for:
  // Inputs, Outputs, 3xMult
  void Correlator::GenZeroPartiesSend() {
    std::size_t degree = mParties - 1;
    std::size_t n_amount = 3*mNMultBatches + mNInOutBatches;
    std::size_t n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);
    for ( std::size_t block = 0; block < n_blocks; block++ ) {
      // 1 sample secret and shares
      Vec secrets(std::vector<FF>(mBatchSize, FF(0)));

      auto poly = scl::details::EvPolyFromSecretsAndDegree(secrets, degree, mPRG);
      auto shares = scl::details::SharesFromEvPoly(poly, mParties);
	
      // 2 send shares
      for ( std::size_t party = 0; party < mParties; party++ ){
	mNetwork->Party(party)->Send(shares[party]);
      }
    }
  }

  void Correlator::GenZeroPartiesReceive() {
    std::size_t n_amount = 3*mNMultBatches + mNInOutBatches;
    std::size_t ctr(0);
    std::size_t n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);
    for ( std::size_t block = 0; block < n_blocks; block++ ) {
      // 1 receive shares
      Vec recv_shares;
      recv_shares.Reserve(mParties);
      for (std::size_t parties = 0; parties < mParties; parties++) {
	FF buffer;
	mNetwork->Party(parties)->Recv(buffer);
	recv_shares.Emplace(buffer);
      }
      // 2 multiply by Vandermonde
      for ( std::size_t shr_idx = 0; shr_idx < mThreshold+1; shr_idx++ ){
	FF shr(0);
	for ( std::size_t j = 0; j < mParties; j++ ){
	  shr += mVandermonde[j][shr_idx] * recv_shares[j];
	}
	if (ctr < mNMultBatches) mMultBatchFIPrep[ctr].mShrO1 = shr; 
	if ((mNMultBatches <= ctr) && (ctr < 2*mNMultBatches)) mMultBatchFIPrep[ctr - mNMultBatches].mShrO2 = shr;
	if ((2*mNMultBatches <= ctr) && (ctr < 3*mNMultBatches)) mMultBatchFIPrep[ctr - 2*mNMultBatches].mShrO3 = shr;
	else {
	  IOBatchFIPrep tmp;
	  tmp.mShrO = shr;
	  mIOBatchFIPrep.emplace_back(tmp);
	}
	ctr++;
      }
    }
  }

  void Correlator::GenZeroForProdPartiesSend() {
    std::size_t degree = mParties - 1;
    std::size_t n_amount = mNMultBatches; 
    std::size_t n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);

    for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
      for ( std::size_t block = 0; block < n_blocks; block++ ) {
	// 1 sample secret and shares
	FF secret(0);

	auto poly = scl::details::EvPolyFromSecretAndPointAndDegree(secret, FF(-pack_idx), degree, mPRG);
	auto shares = scl::details::SharesFromEvPoly(poly, mParties);
	
	// 2 send shares
	for ( std::size_t party = 0; party < mParties; party++ ){
	  mNetwork->Party(party)->Send(shares[party]);
	}
      }
    }
  }

  void Correlator::GenZeroForProdPartiesReceive() {
    std::size_t n_amount = mNMultBatches;
    std::size_t n_blocks = (n_amount + (mThreshold + 1) -1) / (mThreshold + 1);
    mZeroProdShrs.reserve(mBatchSize);

    for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
      mZeroProdShrs.emplace_back(std::vector<FF>());
      mZeroProdShrs[pack_idx].reserve(mNMultBatches);

      for ( std::size_t block = 0; block < n_blocks; block++ ) {
	// 1 receive shares
	Vec recv_shares;
	recv_shares.Reserve(mParties);
	for (std::size_t parties = 0; parties < mParties; parties++) {
	  FF buffer;
	  mNetwork->Party(parties)->Recv(buffer);
	  recv_shares.Emplace(buffer);
	}
	// 2 multiply by Vandermonde
	for ( std::size_t shr_idx = 0; shr_idx < mThreshold+1; shr_idx++ ){
	  FF shr(0);
	  for ( std::size_t j = 0; j < mParties; j++ ){
	    shr += mVandermonde[j][shr_idx] * recv_shares[j];
	  }
	  mZeroProdShrs[pack_idx].emplace_back(shr);
	}
      }
    }
  }

  void Correlator::GenProdPartiesSendP1() {
    for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
      for ( std::size_t batch = 0; batch < mNMultBatches; batch++ ) {
        // 1. Gather shares
	FF share = mUnpackedShrsA[pack_idx][batch] * mUnpackedShrsB[pack_idx][batch]\
	  + mUnpackedShrsMask[pack_idx][batch] + mZeroProdShrs[pack_idx][batch];

	// 2. send shares
	mNetwork->Party(0)->Send(share);
      }
    }
  }

  void Correlator::GenProdP1ReceivesAndSends() {
    if ( mID == 0 ) {
      for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
	for ( std::size_t batch = 0; batch < mNMultBatches; batch++ ) {
	  // 1. Receive shares
	  Vec recv_shares;
	  recv_shares.Reserve(mParties);
	  for (std::size_t parties = 0; parties < mParties; parties++) {
	    FF buffer;
	    mNetwork->Party(parties)->Recv(buffer);
	    recv_shares.Emplace(buffer);
	  }

	  // 2. Reconstruct
	  auto secret = SecretFromPointAndShares(FF(-pack_idx), recv_shares);

	  // 3. Send back (w. optimization of zero-shares)
	  Vec y_points;
	  y_points.Reserve(mThreshold+1);
	  y_points.Emplace(secret);
	  for (std::size_t i = 1; i < mThreshold+1; ++i) y_points.Emplace(FF(0));

	  Vec x_points;
	  x_points.Reserve(mThreshold+1);
	  x_points.Emplace(FF(-pack_idx));
	  for (std::size_t i = 1; i < mThreshold+1; ++i) x_points.Emplace(FF(i));

	  auto poly = scl::details::EvPolynomial<FF>(x_points, y_points);
	  auto shares_to_send = scl::details::SharesFromEvPoly(poly, mParties);
	  for ( std::size_t i = mThreshold; i < mParties; i++ ) {
	    mNetwork->Party(i)->Send(shares_to_send[i]);
	  }
	}
      }
    }
  }

  void Correlator::GenProdPartiesReceive() {
    std::vector<std::vector<FF>> shares_prod;
    shares_prod.reserve(mBatchSize);

    for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
      shares_prod.emplace_back(std::vector<FF>());
      shares_prod[pack_idx].reserve(mNMultBatches);
      for ( std::size_t batch = 0; batch < mNMultBatches; batch++ ) {
	FF recv;
	if (mID >= mThreshold) {
	// 1. Receive secret
	mNetwork->Party(0)->Recv(recv);
	} else {
	  recv = FF(0);
	}
	       
	// 2. Compute shares
	FF shr_prod = recv - mUnpackedShrsMask[pack_idx][batch];
	shares_prod[pack_idx].emplace_back(shr_prod);
      }
    }

    for ( std::size_t i = 0; i < mNMultBatches; i++ ) {
      for ( std::size_t pack_idx = 0; pack_idx < mBatchSize; pack_idx++ ) {
	mMultBatchFIPrep[i].mShrC += mSharesOfEi[pack_idx] * shares_prod[pack_idx][i];
      }
    }
  }
}
