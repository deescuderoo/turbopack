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
	input_gate->SetDummyLambda(lambda);
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
} // namespace tp
