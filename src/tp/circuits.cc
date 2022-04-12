#include "tp/circuits.h"

namespace tp {
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

    Circuit circuit(config.n_parties, config.batch_size);

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
