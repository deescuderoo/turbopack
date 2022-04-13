#include <catch2/catch.hpp>
#include <iostream>

#include "tp/input_gate.h"
#include "tp/output_gate.h"

TEST_CASE("OutputGate") {
  std::size_t threshold = 4; // has to be even
  std::size_t batch_size = (threshold + 2)/2;
  std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
  auto networks = scl::Network::CreateFullInMemory(n_parties);

  SECTION("OutputGate") {
    std::size_t owner_id = 1;
    std::vector<std::shared_ptr<tp::InputGate>> x_gates;
    x_gates.reserve(n_parties);
    tp::FF lambda;

    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates.emplace_back(std::make_shared<tp::InputGate>(0)); // P1 input owner
      x_gates[i]->SetNetwork(std::make_shared<scl::Network>(networks[i]), i);
      x_gates[i]->_DummyPrep(lambda);
    }

    tp::FF X(542);

    std::vector<std::shared_ptr<tp::OutputGate>> z_gates;
    z_gates.reserve(n_parties);

    for (std::size_t i = 0; i < n_parties; i++) {
      z_gates.emplace_back(std::make_shared<tp::OutputGate>(owner_id, x_gates[i])); 
      z_gates[i]->SetNetwork(std::make_shared<scl::Network>(networks[i]), i);
    }

    x_gates[0]->_SetDummyMu(X - lambda);
    (void)z_gates[0]->GetMu();

    // P1 sends mu to owner
    for (std::size_t i = 0; i < n_parties; i++) {
      z_gates[i]->P1SendsMu();
    }

    // Owner receives mu
    for (std::size_t i = 0; i < n_parties; i++) {
      z_gates[i]->OwnerReceivesMu();
    }

    REQUIRE(z_gates[owner_id]->GetValue() == X);
  }

}
