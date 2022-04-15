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

    mIsClosed = false;
    mIsNetworkSet = false;
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

} // namespace tp
