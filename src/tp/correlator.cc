#include "tp/correlator.h"

namespace tp {
      // PREP INPUT BATCH
  void Correlator::PrepInputPartiesSendOwner(std::shared_ptr<InputBatch> input_batch) {
    // 1 collect [lambda_alpha]_n-1
      FF shr_lambdaA_p_R(0);
      for (std::size_t i = 0; i < mBatchSize; i++) {
	shr_lambdaA_p_R += mSharesOfEi[i] * mMapIndShrs[input_batch->GetInputGate(i)];
      }

    // 2 add share of 0
      shr_lambdaA_p_R += mMapInputBatch[input_batch].mShrO;

    // 3 send to Owner
      mNetwork->Party(input_batch->GetOwner())->Send(shr_lambdaA_p_R);
    }
    
    void Correlator::PrepInputOwnerReceives(std::shared_ptr<InputBatch> input_batch) {
      if (mID == input_batch->GetOwner()) {
	Vec recv_shares;
	Vec recv_secret;
	recv_shares.Reserve(mParties);
	recv_secret.Reserve(mBatchSize);

	// Owner receives
	for (std::size_t i = 0; i < mParties; i++) {
	  FF buffer;
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares.Emplace(buffer);
	}
	// TODO watch out for degree
	recv_secret = scl::details::SecretsFromSharesAndLength(recv_shares, mBatchSize);

	// Assign lambdas
	for (std::size_t i = 0; i < mBatchSize; i++) {
	  auto input_gate = input_batch->GetInputGate(i);
	  input_gate->SetLambda(recv_secret[i]);
	}
      }
    }


    // PREP OUTPUT BATCH
    void Correlator::PrepOutputPartiesSendOwner(std::shared_ptr<OutputBatch> output_batch) {
    // 1 collect [lambda_alpha]_n-1
      FF shr_lambdaA_p_R(0);
      for (std::size_t i = 0; i < mBatchSize; i++) {
	shr_lambdaA_p_R += mSharesOfEi[i] * mMapIndShrs[output_batch->GetOutputGate(i)];
      }

    // 2 add share of 0
      shr_lambdaA_p_R += mMapOutputBatch[output_batch].mShrO;

    // 3 send to P1
      mNetwork->Party(output_batch->GetOwner())->Send(shr_lambdaA_p_R);
    }
    
    void Correlator::PrepOutputOwnerReceives(std::shared_ptr<OutputBatch> output_batch) {
      if (mID == output_batch->GetOwner()) {
	Vec recv_shares;
	Vec recv_secret;
	recv_shares.Reserve(mParties);
	recv_secret.Reserve(mBatchSize);

	// Owner receives
	for (std::size_t i = 0; i < mParties; i++) {
	  FF buffer;
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares.Emplace(buffer);
	}
	// TODO watch out for degree
	recv_secret = scl::details::SecretsFromSharesAndLength(recv_shares, mBatchSize);

	// Assign lambdas
	for (std::size_t i = 0; i < mBatchSize; i++) {
	  auto output_gate = output_batch->GetOutputGate(i);
	  output_gate->SetLambda(recv_secret[i]);
	}
      }
    }

    // PREP MULT BATCH
    void Correlator::PrepMultPartiesSendP1(std::shared_ptr<MultBatch> mult_batch) {
    // 1 collect [lambda_alpha]_n-1
      FF shr_lambdaA_p_R(0);
      FF shr_lambdaB_p_R(0);
      for (std::size_t i = 0; i < mBatchSize; i++) {
	shr_lambdaA_p_R += mSharesOfEi[i] * mMapIndShrs[mult_batch->GetMultGate(i)->GetLeft()];
	shr_lambdaB_p_R += mSharesOfEi[i] * mMapIndShrs[mult_batch->GetMultGate(i)->GetRight()];
      }

    // 2 get random sharing [r]_n-1 and add [lambda_alpha]_n-1 + [r]_n-1
      shr_lambdaA_p_R += mMapMultBatch[mult_batch].mShrA + mMapMultBatch[mult_batch].mShrO1;
      shr_lambdaB_p_R += mMapMultBatch[mult_batch].mShrB + mMapMultBatch[mult_batch].mShrO2;

    // 3 send to P1
      mNetwork->Party(0)->Send(shr_lambdaA_p_R);
      mNetwork->Party(0)->Send(shr_lambdaB_p_R);
    }
    
    void Correlator::PrepMultP1ReceivesAndSends() {
      if (mID == 0) {
	Vec recv_shares_A;
	Vec recv_secret_A;
	Vec recv_shares_B;
	Vec recv_secret_B;
	recv_shares_A.Reserve(mParties);
	recv_secret_A.Reserve(mBatchSize);
	recv_shares_B.Reserve(mParties);
	recv_secret_B.Reserve(mBatchSize);

	// P1 receives
	for (std::size_t i = 0; i < mParties; i++) {
	  FF buffer;
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares_A.Emplace(buffer);
	  mNetwork->Party(i)->Recv(buffer);
	  recv_shares_B.Emplace(buffer);
	}
	recv_secret_A = scl::details::SecretsFromSharesAndLength(recv_shares_A, mBatchSize);
	recv_secret_B = scl::details::SecretsFromSharesAndLength(recv_shares_B, mBatchSize);

	// P1 generates new shares
	auto poly_A = scl::details::EvPolyFromSecretsAndDegree(recv_secret_A, mBatchSize-1, mPRG);
	auto poly_B = scl::details::EvPolyFromSecretsAndDegree(recv_secret_B, mBatchSize-1, mPRG);
	Vec new_shares_A = scl::details::SharesFromEvPoly(poly_A, mParties);
	Vec new_shares_B = scl::details::SharesFromEvPoly(poly_B, mParties);

	// P1 sends
	for (std::size_t i = 0; i < mParties; ++i) {
	  mNetwork->Party(i)->Send(new_shares_A[i]);
	  mNetwork->Party(i)->Send(new_shares_B[i]);
	}
      }
    }

    void Correlator::PrepMultPartiesReceive(std::shared_ptr<MultBatch> mult_batch) {
      // Receive
      FF recv_share_A;
      FF recv_share_B;
      mNetwork->Party(0)->Recv(recv_share_A);
      mNetwork->Party(0)->Recv(recv_share_B);

      // Subtract shares of [r]_n-k
      FF new_share_A = recv_share_A - mMapMultBatch[mult_batch].mShrA;
      FF new_share_B = recv_share_B - mMapMultBatch[mult_batch].mShrB;

      // Set deltas
      FF shr_delta(0);
      for (std::size_t i = 0; i < mBatchSize; i++) {
	shr_delta -= mSharesOfEi[i] * mMapIndShrs[mult_batch->GetMultGate(i)];
      }
      shr_delta += recv_share_A * recv_share_B - recv_share_A * mMapMultBatch[mult_batch].mShrB \
	- recv_share_B * mMapMultBatch[mult_batch].mShrA + mMapMultBatch[mult_batch].mShrC \
	+ mMapMultBatch[mult_batch].mShrO3;

      // Set preprocessing
      mult_batch->SetPreprocessing(new_share_A, new_share_B, shr_delta);

    }
}

