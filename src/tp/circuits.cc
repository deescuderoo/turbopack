#include "tp/circuits.h"

namespace tp {
  void Circuit::Init(std::size_t n_clients, std::size_t batch_size) {
    // Initialize input layer
    mInputLayers.reserve(n_clients);
    mFlatInputGates.resize(n_clients);
    for (std::size_t i = 0; i < n_clients; i++) {
      auto input_layer = InputLayer(i, batch_size);
      mInputLayers.emplace_back(input_layer);
    }
      
    // Initialize output layer
    mOutputLayers.reserve(n_clients);
    mFlatOutputGates.resize(n_clients);
    for (std::size_t i = 0; i < n_clients; i++) {
      auto output_layer = OutputLayer(i, batch_size);
      mOutputLayers.emplace_back(output_layer);
    }

    // Initialize first mult layer
    auto first_layer = MultLayer(mBatchSize);
    mMultLayers.emplace_back(first_layer);
    mFlatMultLayers.emplace_back(VecMultGates());
  }

  std::shared_ptr<InputGate> Circuit::Input(std::size_t owner_id) {
    auto input_gate = std::make_shared<tp::InputGate>(owner_id);
    mInputLayers[owner_id].Append(input_gate);
    mFlatInputGates[owner_id].emplace_back(input_gate);
    mInputGates.emplace_back(input_gate);
    return input_gate;
  }

  std::shared_ptr<OutputGate> Circuit::Output(std::size_t owner_id, std::shared_ptr<Gate> output) {
    auto output_gate = std::make_shared<tp::OutputGate>(owner_id, output);
    mOutputLayers[owner_id].Append(output_gate);
    mFlatOutputGates[owner_id].emplace_back(output_gate);
    mOutputGates.emplace_back(output_gate);
    return output_gate;
  }

  std::shared_ptr<MultGate> Circuit::Mult(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
    auto mult_gate = std::make_shared<tp::MultGate>(left, right);
    mMultLayers.back().Append(mult_gate);
    mFlatMultLayers.back().emplace_back(mult_gate);
    mSize++;
    return mult_gate;
  }

  void Circuit::NewLayer() {
    // Update width
    if (mWidth < mFlatMultLayers.back().size()) mWidth = mFlatMultLayers.back().size();
    mFlatMultLayers.emplace_back(VecMultGates());
      
    // Pad the batches if necessary
    mMultLayers.back().Close();
    // Open space for the next layer
    auto next_layer = MultLayer(mBatchSize);
    mMultLayers.emplace_back(next_layer);
  }

  void Circuit::LastLayer() {
    // Update width
    if (mWidth < mFlatMultLayers.back().size()) mWidth = mFlatMultLayers.back().size();
    mFlatMultLayers.emplace_back(VecMultGates());

    mMultLayers.back().Close();
  }

  void Circuit::SetClearInputs(std::vector<std::vector<FF>> inputs_per_client) {
    if ( inputs_per_client.size() != mClients )
      throw std::invalid_argument("Number of clients do not match");

    for (std::size_t i = 0; i < mClients; i++) {
      if ( inputs_per_client[i].size() != mFlatInputGates[i].size() )
	throw std::invalid_argument("Number of inputs provided for a client does not match its number of input gates");

      for (std::size_t j = 0; j < inputs_per_client[i].size(); j++) mFlatInputGates[i][j]->ClearInput(inputs_per_client[i][j]);
    }
  }
  
  void Circuit::SetClearInputsFlat(std::vector<FF> inputs) {
    std::size_t idx(0);
    for (std::size_t i = 0; i < mClients; i++) {
      for (auto input_gate : mFlatInputGates[i]) {
	input_gate->ClearInput(inputs[idx]);
	idx++;
      }
    }
  }
  
  std::vector<std::vector<FF>> Circuit::GetClearOutputs() {
    std::vector<std::vector<FF>> output;
    output.reserve(mClients);
    for (std::size_t i = 0; i < mClients; i++) {
      std::vector<FF> output_i;
      output_i.reserve(mFlatOutputGates[i].size());
      for (auto output_gate : mFlatOutputGates[i]) output_i.emplace_back(output_gate->GetClear());
      output.emplace_back(output_i);
    }
    return output;
  }

  std::vector<FF> Circuit::GetClearOutputsFlat() {
    std::vector<FF> output;
    for (auto output_gate : mOutputGates) output.emplace_back(output_gate->GetClear());
    return output;
  }


  Circuit Circuit::FromConfig(CircuitConfig config) {
    // Validate input

    std::size_t n_inputs = config.ComputeNumberOfInputs();
    if ( config.n_parties != config.inp_gates.size() )
      throw std::invalid_argument("The number of parties does not coincide with the length of the input vector");
    if ( config.n_parties != config.out_gates.size() )
      throw std::invalid_argument("The number of parties does not coincide with the length of the output vector");
    if ( config.width % n_inputs != 0 )
      throw std::invalid_argument("Total number of inputs does not divide width");
    if ( n_inputs % 2 != 0 )
      throw std::invalid_argument("Number of inputs is not even");
    if ( config.batch_size % 2 != 0 )
      throw std::invalid_argument("Batch size is not even");

    Circuit circuit(config.n_parties, config.batch_size);

    // ASSEMBLE CIRCUIT

    // Input gates
    for (std::size_t i = 0; i < config.n_parties; i++) {
      for (std::size_t j = 0; j < config.inp_gates[i]; j++) {
	circuit.Input(i);
      }
    }
    circuit.CloseInputs();

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
    for (std::size_t i = 0; i < config.n_parties; i++) {
      for (std::size_t j = 0; j < config.out_gates[i]; j++) {
	auto x = circuit.GetMultGate(config.depth - 1, i % config.width);
	circuit.Output(i,x);
      }
    }
    circuit.CloseOutputs();
    
    return circuit;
  };
} // namespace tp
