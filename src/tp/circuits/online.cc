#include "tp/circuits.h"

namespace tp {
  void Circuit::SetInputs(std::vector<FF> inputs) {
      if ( inputs.size() != mFlatInputGates[mID].size() )
	throw std::invalid_argument("Number of inputs do not match");
      // Set input and send to P1
      for (std::size_t i = 0; i < inputs.size(); i++) {
	mFlatInputGates[mID][i]->SetInput(inputs[i]);
      }      
    }

    // Input protocol
    void Circuit::InputOwnerSendsP1() {
      for (std::size_t i = 0; i < mClients; i++) {
	for (auto input_gate : mFlatInputGates[i]) {
	  input_gate->OwnerSendsP1();
	}
      }
    }
    void Circuit::InputP1Receives() {
      for (std::size_t i = 0; i < mClients; i++) { // outer loop can be removed if needed for opt.
	for (auto input_gate : mFlatInputGates[i]) {
	  input_gate->P1Receives();
	}
      }
    }
    void Circuit::RunInput() {
      InputOwnerSendsP1();
      InputP1Receives();
    }

    // Multiplications in the i-th layer
    void Circuit::MultP1Sends(std::size_t layer) { mMultLayers[layer].P1Sends(); }
    void Circuit::MultPartiesReceive(std::size_t layer) { mMultLayers[layer].PartiesReceive(); }
    void Circuit::MultPartiesSend(std::size_t layer) { mMultLayers[layer].PartiesSend(); }
    void Circuit::MultP1Receives(std::size_t layer) { mMultLayers[layer].P1Receives(); }

    void Circuit::RunMult(std::size_t layer) {
      MultP1Sends(layer);
      MultPartiesReceive(layer);
      MultPartiesSend(layer);
      MultP1Receives(layer);
    }

    // Output layers
    void Circuit::OutputP1SendsMu() {
      for (std::size_t i = 0; i < mClients; i++) {
	for (auto output_gate : mFlatOutputGates[i]) {
	  output_gate->P1SendsMu();
	}
      }
    }
    void Circuit::OutputOwnerReceivesMu() {
      for (std::size_t i = 0; i < mClients; i++) { // outer loop can be removed if needed for opt.
	for (auto output_gate : mFlatOutputGates[i]) {
	  output_gate->OwnerReceivesMu();
	}
      }
    }
    void Circuit::RunOutput() {
      OutputP1SendsMu();
      OutputOwnerReceivesMu();
    }

    void Circuit::RunProtocol() {
      RunInput();
      for (std::size_t layer = 0; layer < mMultLayers.size(); layer++) {
	RunMult(layer);
      }
      RunOutput();
    }
    
    // Returns a vector with the outputs after computation
    std::vector<FF> Circuit::GetOutputs() {
      std::vector<FF> output;
      output.reserve(mFlatOutputGates[mID].size());
      for (auto output_gate : mFlatOutputGates[mID]) {
	output.emplace_back(output_gate->GetValue());
      }
      return output;
    }
} // namespace tp
