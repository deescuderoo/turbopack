#include "tp/circuits.h"

namespace tp {
    // Populates each batch with dummy preprocessing (all zeros)
  void Circuit::_DummyPrep() {
      for (auto input_layer : mInputLayers) input_layer._DummyPrep();
      for (auto mult_layer : mMultLayers) mult_layer._DummyPrep();
      for (auto output_layer : mOutputLayers) output_layer._DummyPrep();
    }

    // Set all settable lambdas to a constant
    void Circuit::SetDummyLambdas(FF lambda) {
      // Output wires of multiplications
      for (std::size_t layer = 0; layer < GetDepth(); layer++) {
	for (auto mult_gate : mFlatMultLayers[layer]) {
	  mult_gate->SetDummyLambda(lambda);
	}
      }
      // Output wires of input gates
      for (auto input_gate : mInputGates) {
	input_gate->SetLambda(lambda);
      }
    }

    // Populates the lambdas of addition and output gates
    void Circuit::PopulateDummyLambdas() {
      // Addition gates
      for (auto add_gate : mAddGates) {
	(void)add_gate->GetDummyLambda();
      }
      // Output gates
      for (auto output_gate : mOutputGates) {
	(void)output_gate->GetDummyLambda();
      }      
    }

    // Sets the lambdas for the output wires of addition and output
    // gates based on the lambdas for multiplications and input gates
    void Circuit::PrepFromDummyLambdas() {
      for (auto input_layer : mInputLayers) input_layer.PrepFromDummyLambdas();
      for (auto output_layer : mOutputLayers) output_layer.PrepFromDummyLambdas();
      for (auto mult_layer : mMultLayers) mult_layer.PrepFromDummyLambdas();
    }

    void Circuit::GenCorrelator() {
      if ( !mIsNetworkSet )
	throw std::invalid_argument("Cannot set correlator without setting a network first");
      
      std::size_t n_ind_shares = GetNInputs() + GetSize();
      std::size_t n_mult_batches = GetNMultBatches();
      std::size_t n_inout_batches = GetNInputBatches() + GetNOutputBatches();

      mCorrelator = Correlator(n_ind_shares, n_mult_batches, n_inout_batches, mBatchSize);
      mCorrelator.SetNetwork(mNetwork, mID);
      mCorrelator.PrecomputeEi();
    }

    // Populates the mappings in the correlator so that the
    // F.I. preprocessing is mapped to different gates/batches
    void Circuit::MapCorrToCircuit() {
      // Individual shares
      for (auto input_gate : mInputGates) {
	mCorrelator.PopulateIndvShrs(input_gate);
      }
      for (std::size_t layer = 0; layer < GetDepth(); layer++) {
	for (auto mult_gate : mFlatMultLayers[layer]) {
	  mCorrelator.PopulateIndvShrs(mult_gate);
	}
      }
      for (auto add_gate : mAddGates) {
	mCorrelator.PopulateIndvShrs(add_gate);
      }
      for (auto output_gate : mOutputGates) {
	mCorrelator.PopulateIndvShrs(output_gate);
      }

      // Input batches
      for (auto input_layer : mInputLayers) {
	for (auto input_batch : input_layer.mBatches) {
	  mCorrelator.PopulateInputBatches(input_batch);
	}
      }
      // Output batches
      for (auto output_layer : mOutputLayers) {
	for (auto output_batch : output_layer.mBatches) {
	  mCorrelator.PopulateOutputBatches(output_batch);
	}
      }
      // Mult batches
      for (auto mult_layer : mMultLayers) {
	for (auto mult_batch : mult_layer.mBatches) {
	  mCorrelator.PopulateMultBatches(mult_batch);
	}
      }
    }

    // Prep inputs & outputs
    void Circuit::PrepMultPartiesSendP1() {
      for (auto mult_layer : mMultLayers) {
	for (auto mult_batch : mult_layer.mBatches) {
	  mCorrelator.PrepMultPartiesSendP1(mult_batch);
	}
      }
    }
    void Circuit::PrepMultP1ReceivesAndSends() {
      for (auto mult_layer : mMultLayers) {
	for (auto mult_batch : mult_layer.mBatches) {
	  (void) mult_batch;
	  mCorrelator.PrepMultP1ReceivesAndSends();
	}
      }      
    }
    void Circuit::PrepMultPartiesReceive() {
      for (auto mult_layer : mMultLayers) {
	for (auto mult_batch : mult_layer.mBatches) {
	  mCorrelator.PrepMultPartiesReceive(mult_batch);
	}
      }
    }

    void Circuit::PrepIOPartiesSendOwner() {
      for (auto input_layer : mInputLayers) {
	for (auto input_batch : input_layer.mBatches) {
	  mCorrelator.PrepInputPartiesSendOwner(input_batch);
	}
      }
      for (auto output_layer : mOutputLayers) {
	for (auto output_batch : output_layer.mBatches) {
	  mCorrelator.PrepOutputPartiesSendOwner(output_batch);
	}
      }
    }

    void Circuit::PrepIOOwnerReceives() {
      for (auto input_layer : mInputLayers) {
	for (auto input_batch : input_layer.mBatches) {
	  mCorrelator.PrepInputOwnerReceives(input_batch);
	}
      }
      for (auto output_layer : mOutputLayers) {
	for (auto output_batch : output_layer.mBatches) {
	  mCorrelator.PrepOutputOwnerReceives(output_batch);
	}
      }
    }
  
} // namespace tp
