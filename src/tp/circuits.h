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
    std::size_t n_outputs;

    std::size_t ComputeNumberOfInputs() {
      std::size_t sum(0);
      for (auto n_inputs : inp_gates)
	sum += n_inputs;
      return sum;
    }
    std::size_t ComputeNumberOfOutputs() {
      std::size_t sum(0);
      for (auto n_inputs : out_gates)
	sum += n_inputs;
      return sum;
    }
  };

  class Circuit {
  public:
    Circuit(std::size_t batch_size) : mBatchSize(batch_size) {
      auto first_layer = Layer(mBatchSize);
      mLayers.emplace_back(first_layer);

      mFlatLayers.emplace_back(VecMultGates());
    }

    Circuit() : mBatchSize(1) {
      auto first_layer = Layer(mBatchSize);
      mLayers.emplace_back(first_layer);

      mFlatLayers.emplace_back(VecMultGates());
      }

    // Append input gates
    std::shared_ptr<InputGate> Input(std::size_t owner_id) {
      auto ptr = std::make_shared<tp::InputGate>(owner_id);
      mInputGates.emplace_back(ptr);
      return ptr;
    }

    // Append output gates
    std::shared_ptr<OutputGate> Output(std::shared_ptr<Gate> output) {
      auto output_gate = std::make_shared<tp::OutputGate>(output);
      mOutputGates.emplace_back(output_gate);
      return output_gate;
    }

    // Append addition gates
    std::shared_ptr<AddGate> Add(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      return std::make_shared<tp::AddGate>(left, right);
    }

    // Append multiplication gates. These are added to the current
    // layer, which in turns add them to the current batch within that layer.
    std::shared_ptr<MultGate> Mult(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      auto mult_gate = std::make_shared<tp::MultGate>(left, right);
      mLayers.back().Append(mult_gate);
      mFlatLayers.back().emplace_back(mult_gate);
      mSize++;
      return mult_gate;
    }

    // Used after the multiplications of a given layer have been
    // added, and new multiplications will be added to the next
    // layer. This closes the current layer, meaning that dummy gates
    // are appended if necessary to reach a full batch 
    void NewLayer() {
      // Update width
      if (mWidth < mFlatLayers.back().size()) mWidth = mFlatLayers.back().size();
      mFlatLayers.emplace_back(VecMultGates());
      
      // Pad the batches if necessary
      mLayers.back().Close();
      // Open space for the next layer
      auto next_layer = Layer(mBatchSize);
      mLayers.emplace_back(next_layer);
    }

    // Closes the current layer and does not open a new one. 
    void LastLayer() {
      // Update width
      if (mWidth < mFlatLayers.back().size()) mWidth = mFlatLayers.back().size();
      mFlatLayers.emplace_back(VecMultGates());

      mLayers.back().Close();
    }

    // Populates each batch with dummy preprocessing (all zeros)
    void DummyPrep() {
      for (auto layer : mLayers) layer._DummyPrep();
    }

    // Used to fetch input and output gates
    std::shared_ptr<InputGate> GetInputGate(std::size_t idx) { return mInputGates[idx]; }
    std::shared_ptr<OutputGate> GetOutputGate(std::size_t idx) { return mOutputGates[idx]; }

    // Used to fetch the idx-th mult gate of the desired layer
    std::shared_ptr<MultGate> GetMultGate(std::size_t layer, std::size_t idx) {
      auto out = mFlatLayers[layer][idx];
      // auto out2 = mLayers[layer].GetMultBatch(idx/mBatchSize)->GetMultGate(idx % mBatchSize);
      // assert(mLayers[layer].GetMultBatch(idx/mBatchSize)->GetMultGate(idx % mBatchSize) == out); // PASSES
      return out;
    }

    // Evaluates the circuit in the clear on the given inputs
    std::vector<FF> ClearEvaluation(std::vector<FF> inputs) {
      // 1. Set inputs
      if ( inputs.size() != mInputGates.size() )
	throw std::invalid_argument("Number of inputs provided does not match number of input gates");
      for (std::size_t i = 0; i < inputs.size(); i++) mInputGates[i]->ClearInput(inputs[i]);

      // 2. Evaluate last layer, which evaluates all others
      mLayers.back().ClearEvaluation();

      // 3. Evaluate output gates
      std::vector<FF> output;
      for (auto output_gate : mOutputGates) output.emplace_back(output_gate->GetClear());
      return output;
    }

    // Generates a synthetic circuit with the desired metrics
    static Circuit FromConfig(CircuitConfig config);

    // Fetch metrics
    std::size_t GetDepth() { return mLayers.size(); }
    std::size_t GetNInputs() { return mInputGates.size(); }
    std::size_t GetNOutputs() { return mOutputGates.size(); }
    std::size_t GetWidth() { return mWidth; }
    std::size_t GetSize() { return mSize; }

  private:
    std::size_t mBatchSize;

    // List of layers. Each layer is itself a list of batches, which
    // is itself a list of batch_size multiplications
    std::vector<Layer> mLayers;
    // List of layers where each layer contains the multiplications
    // themselves, not the batches
    std::vector<std::vector<std::shared_ptr<MultGate>>> mFlatLayers;

    // Lists with input and output gates
    std::vector<std::shared_ptr<InputGate>> mInputGates;
    std::vector<std::shared_ptr<OutputGate>> mOutputGates;

    // Metrics
    std::size_t mWidth=0;
    std::size_t mSize=0; // number of multiplications
  };



} // namespace tp

#endif  // CIRCUIT_H
