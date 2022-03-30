#include <catch2/catch.hpp>
#include <iostream>
#include <assert.h>

#include "tp/gate.h"

using VecMultGates = std::vector<std::shared_ptr<tp::MultGate>>;

namespace tp {

  struct CircuitConfig {
    std::vector<std::size_t> inp_gates;
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
    
    std::shared_ptr<InputGate> Input(std::size_t owner_id) {
      auto ptr = std::make_shared<tp::InputGate>(owner_id);
      mInputGates.emplace_back(ptr);
      return ptr;
    }

    std::shared_ptr<OutputGate> Output(std::shared_ptr<Gate> output) {
      auto output_gate = std::make_shared<tp::OutputGate>(output);
      mOutputGates.emplace_back(output_gate);
      return output_gate;
    }

    std::shared_ptr<AddGate> Add(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      return std::make_shared<tp::AddGate>(left, right);
    }

    std::shared_ptr<MultGate> Mult(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      auto mult_gate = std::make_shared<tp::MultGate>(left, right);
      mLayers.back().Append(mult_gate);
      mFlatLayers.back().emplace_back(mult_gate);
      mSize++;
      return mult_gate;
    }

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

    void LastLayer() {
      // Update width
      if (mWidth < mFlatLayers.back().size()) mWidth = mFlatLayers.back().size();
      mFlatLayers.emplace_back(VecMultGates());

      mLayers.back().Close();
    }

    void DummyPrep() {
      for (auto layer : mLayers) layer.DummyPrep();
    }

    std::shared_ptr<InputGate> GetInputGate(std::size_t idx) { return mInputGates[idx]; }
    std::shared_ptr<OutputGate> GetOutputGate(std::size_t idx) { return mOutputGates[idx]; }

    std::shared_ptr<MultGate> GetMultGate(std::size_t layer, std::size_t idx) {
      auto out = mFlatLayers[layer][idx];
      auto out2 = mLayers[layer].GetMultBatch(idx/mBatchSize)->GetMultGate(idx % mBatchSize);
      // assert(mLayers[layer].GetMultBatch(idx/mBatchSize)->GetMultGate(idx % mBatchSize) == out); // PASSES
      return out;
    }

    std::vector<FF> GetClear(std::vector<FF> inputs) {
      // 1. Set inputs
      if ( inputs.size() != mInputGates.size() )
	throw std::invalid_argument("Number of inputs provided does not match number of input gates");
      for (std::size_t i = 0; i < inputs.size(); i++) mInputGates[i]->ClearInput(inputs[i]);

      // 2. Evaluate last layer, which evaluates all others
      mLayers.back().GetClear();

      // 3. Evalute output gates
      std::vector<FF> output;
      for (auto output_gate : mOutputGates) output.emplace_back(output_gate->GetClear());
      return output;
    }

    static Circuit FromConfig(CircuitConfig config);

    // Metrics

    std::size_t GetDepth() { return mLayers.size(); }
    std::size_t GetNInputs() { return mInputGates.size(); }
    std::size_t GetNOutputs() { return mOutputGates.size(); }
    std::size_t GetWidth() { return mWidth; }
    std::size_t GetSize() { return mSize; }

  private:
    std::size_t mBatchSize;

    std::vector<Layer> mLayers;
    std::vector<std::vector<std::shared_ptr<MultGate>>> mFlatLayers;

    std::vector<std::shared_ptr<InputGate>> mInputGates;
    std::vector<std::shared_ptr<OutputGate>> mOutputGates;

    // Metrics
    std::size_t mWidth=0;
    std::size_t mSize=0;
  };


  Circuit Circuit::FromConfig(CircuitConfig config) {
    // Validate input

    std::size_t n_inputs = config.ComputeNumberOfInputs();
    if ( config.n_parties != config.inp_gates.size() )
      throw std::invalid_argument("The number of parties does not coincide with the length the input vector");
    if ( config.width % n_inputs != 0 )
      throw std::invalid_argument("Total number of inputs does not divide width");
    if ( n_inputs % 2 != 0 )
      throw std::invalid_argument("Number of inputs is not even");
    if ( config.width % config.batch_size != 0 )
      throw std::invalid_argument("Batch size does not divide width");
    if ( config.batch_size % 2 != 0 )
      throw std::invalid_argument("Batch size is not even");
    if ( config.n_outputs > config.width )
      throw std::invalid_argument("More output gates than width");

    Circuit circuit(config.batch_size);

    // ASSEMBLE CIRCUIT

    // Input gates
    for (std::size_t i = 0; i < config.n_parties; i++) {
      for (std::size_t j = 0; j < config.inp_gates[i]; j++) {
	circuit.Input(i);
      }
    }

    // First layer
    for (std::size_t i = 0; i < config.width/2; i++) {
      auto x = circuit.GetInputGate((2*i) % n_inputs);
      auto y = circuit.GetInputGate((2*i+1) % n_inputs);
      auto add = circuit.Add(x, y);
      circuit.Mult(x, add);
      circuit.Mult(y, add);
    }

    // Next layers
    for (std::size_t d = 1; d < config.depth; d++) {
      circuit.NewLayer();
      for (std::size_t i = 0; i < config.width/2; i++) {
	auto x = circuit.GetMultGate(d-1, 2*i);
	auto y = circuit.GetMultGate(d-1, 2*i+1);
	auto add = circuit.Add(x, y);
	circuit.Mult(x, add);
	circuit.Mult(y, add);
      }
    }

    // Close layers
    circuit.LastLayer();

    // Output gates
    for (std::size_t i = 0; i < config.n_outputs; i++) {
	auto x = circuit.GetMultGate(config.depth - 1, i);
	circuit.Output(x);
    }
    
    return circuit;
  };

} // namespace tp
