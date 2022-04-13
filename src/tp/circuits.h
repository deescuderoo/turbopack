#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <catch2/catch.hpp>
#include <iostream>
#include <assert.h>

#include "tp/mult_gate.h"
#include "tp/input_gate.h"
#include "tp/output_gate.h"

using VecMultGates = std::vector<std::shared_ptr<tp::MultGate>>;

namespace tp {

  struct CircuitConfig {
    std::vector<std::size_t> inp_gates;
    std::vector<std::size_t> out_gates;
    std::size_t n_parties;
    std::size_t width;
    std::size_t depth;
    std::size_t batch_size;

    std::size_t ComputeNumberOfInputs() {
      std::size_t sum(0);
      for (auto n_inputs : inp_gates)
	sum += n_inputs;
      return sum;
    }
    std::size_t ComputeNumberOfOutputs() {
      std::size_t sum(0);
      for (auto n_outputs : out_gates)
	sum += n_outputs;
      return sum;
    }
  };

  class Circuit {
  public:
    // Initialize input, output and first mult layer
    void Init(std::size_t n_clients, std::size_t batch_size);

    Circuit(std::size_t n_clients, std::size_t batch_size) : mBatchSize(batch_size), mClients(n_clients) {
      Init(n_clients, batch_size);
    }

    Circuit() : mBatchSize(1), mClients(1) {
      Init(mClients, mBatchSize);
      }

    // Append input gates
    std::shared_ptr<InputGate> Input(std::size_t owner_id);

    // Consolidates the batches for the inputs
    void CloseInputs() {
      for (auto input_layer : mInputLayers) input_layer.Close();
    }

    // Append output gates
    std::shared_ptr<OutputGate> Output(std::size_t owner_id, std::shared_ptr<Gate> output);

    // Consolidates the batches for the outputs
    void CloseOutputs() {
      for (auto output_layer : mOutputLayers) output_layer.Close();
    }

    // Append addition gates
    std::shared_ptr<AddGate> Add(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      return std::make_shared<tp::AddGate>(left, right);
    }

    // Append multiplication gates. These are added to the current
    // layer, which in turns add them to the current batch within that layer.
    std::shared_ptr<MultGate> Mult(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right);

    // Used after the multiplications of a given layer have been
    // added, and new multiplications will be added to the next
    // layer. This closes the current layer, meaning that dummy gates
    // are appended if necessary to reach a full batch 
    void NewLayer();

    // Closes the current layer and does not open a new one. 
    void LastLayer();

    // Populates each batch with dummy preprocessing
    void _DummyPrep() {
      for (auto input_layer : mInputLayers) input_layer._DummyPrep();
      for (auto mult_layer : mMultLayers) mult_layer._DummyPrep();
      for (auto output_layer : mOutputLayers) output_layer._DummyPrep();
    }

    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      mNetwork = network;
      mID = id;
      mParties = network->Size();
      for (auto input_layer : mInputLayers) input_layer.SetNetwork(network, id);
      for (auto mult_layer : mMultLayers) mult_layer.SetNetwork(network, id);
      for (auto output_layer : mOutputLayers) output_layer.SetNetwork(network, id);
    }

    // Used to fetch input and output gates
    std::shared_ptr<InputGate> GetInputGate(std::size_t owner_id, std::size_t idx) { return mFlatInputGates[owner_id][idx]; }
    std::shared_ptr<InputGate> GetInputGate(std::size_t idx) { return mInputGates[idx]; }
    std::shared_ptr<OutputGate> GetOutputGate(std::size_t owner_id, std::size_t idx) { return mFlatOutputGates[owner_id][idx]; }
    std::shared_ptr<OutputGate> GetOutputGate(std::size_t idx) { return mOutputGates[idx]; }

    // Used to fetch the idx-th mult gate of the desired layer
    std::shared_ptr<MultGate> GetMultGate(std::size_t layer, std::size_t idx) {
      auto out = mFlatMultLayers[layer][idx];
      // auto out2 = mLayers[layer].GetMultBatch(idx/mBatchSize)->GetMultGate(idx % mBatchSize);
      // assert(mLayers[layer].GetMultBatch(idx/mBatchSize)->GetMultGate(idx % mBatchSize) == out); // PASSES
      return out;
    }

    // Set inputs for evaluations in the clear. Input is a vector of
    // vectors indicating the inputs of each party
    void SetClearInputs(std::vector<std::vector<FF>> inputs_per_client);

    // Set inputs for evaluations in the clear. Input is a vector of
    // inputs which get spread across all parties.  It is NOT checked
    // if there are enough inputs. This may segfault
    void SetClearInputsFlat(std::vector<FF> inputs);

    // Returns a vector of vectors with the outputs of each client
    std::vector<std::vector<FF>> GetClearOutputs();

    // Returns one long vector with all the outputs
    std::vector<FF> GetClearOutputsFlat();

    // Returns a vector with the outputs after computation
    std::vector<FF> GetOutput() {
      std::vector<FF> output;
      output.reserve(mFlatOutputGates[mID].size());
      for (auto output_gate : mFlatOutputGates[mID]) {
	output.emplace_back(output_gate->GetValue());
      }
      return output;
    }

    // PROTOCOLS

    // Set inputs
    // Called separately by each party
    void SetInputs(std::vector<FF> inputs) {
      if ( inputs.size() != mFlatInputGates[mID].size() )
	throw std::invalid_argument("Number of inputs do not match");
      // Set input and send to P1
      for (std::size_t i = 0; i < inputs.size(); i++) {
	mFlatInputGates[mID][i]->SetInput(inputs[i]);
      }      
    }

    // Input protocol
    void InputOwnerSendsP1() {
      for (std::size_t i = 0; i < mClients; i++) {
	for (auto input_gate : mFlatInputGates[i]) {
	  input_gate->OwnerSendsP1();
	}
      }
    }
    void InputP1Receives() {
      for (std::size_t i = 0; i < mClients; i++) { // outer loop can be removed if needed for opt.
	for (auto input_gate : mFlatInputGates[i]) {
	  input_gate->P1Receives();
	}
      }
    }
    void RunInput() {
      InputOwnerSendsP1();
      InputP1Receives();
    }

    // Multiplications in the i-th layer
    void MultP1Sends(std::size_t layer) { mMultLayers[layer].P1Sends(); }
    void MultPartiesReceive(std::size_t layer) { mMultLayers[layer].PartiesReceive(); }
    void MultPartiesSend(std::size_t layer) { mMultLayers[layer].PartiesSend(); }
    void MultP1Receives(std::size_t layer) { mMultLayers[layer].P1Receives(); }

    void RunMult(std::size_t layer) {
      MultP1Sends(layer);
      MultPartiesReceive(layer);
      MultPartiesSend(layer);
      MultP1Receives(layer);
    }

    // Output layers
    void OutputP1SendsMu() {
      for (std::size_t i = 0; i < mClients; i++) {
	for (auto output_gate : mFlatOutputGates[i]) {
	  output_gate->P1SendsMu();
	}
      }
    }
    void OutputOwnerReceivesMu() {
      for (std::size_t i = 0; i < mClients; i++) { // outer loop can be removed if needed for opt.
	for (auto output_gate : mFlatOutputGates[i]) {
	  output_gate->OwnerReceivesMu();
	}
      }
    }
    void RunOutput() {
      OutputP1SendsMu();
      OutputOwnerReceivesMu();
    }

    void RunProtocol() {
      RunInput();
      for (std::size_t layer = 0; layer < mMultLayers.size(); layer++) {
	RunMult(layer);
      }
      RunOutput();
    }
    
    // Generates a synthetic circuit with the desired metrics
    static Circuit FromConfig(CircuitConfig config);

    // Fetch metrics
    std::size_t GetDepth() { return mMultLayers.size(); }
    std::size_t GetNInputs() { return mInputGates.size(); }
    std::size_t GetNOutputs() { return mOutputGates.size(); }
    std::size_t GetWidth() { return mWidth; }
    std::size_t GetSize() { return mSize; }

  private:
    std::size_t mBatchSize;

    // List of layers. Each layer is itself a list of batches, which
    // is itself a list of batch_size gates
    std::vector<InputLayer> mInputLayers; // indexes represent parties
    std::vector<OutputLayer> mOutputLayers; // indexes represent parties
    std::vector<MultLayer> mMultLayers; // indexes represent layers
    
    // List of layers where each layer contains the gates
    // themselves, not the batches
    std::vector<std::vector<std::shared_ptr<MultGate>>> mFlatMultLayers; // Outer idx: layer
    std::vector<std::vector<std::shared_ptr<InputGate>>> mFlatInputGates; // Outer idx: client
    std::vector<std::vector<std::shared_ptr<OutputGate>>> mFlatOutputGates; // Outer idx: client

    // Lists with input and output gates
    std::vector<std::shared_ptr<InputGate>> mInputGates;
    std::vector<std::shared_ptr<OutputGate>> mOutputGates;

    // Metrics
    std::size_t mClients; // number of clients
    std::size_t mWidth=0;
    std::size_t mSize=0; // number of multiplications

    // Network-related
    std::shared_ptr<scl::Network> mNetwork;
    std::size_t mID;
    std::size_t mParties;
  };



} // namespace tp

#endif  // CIRCUIT_H
