#include <catch2/catch.hpp>
#include <iostream>

#include "tp/input_gate.h"
#include "tp/gate.h" // add_gate is defined here

TEST_CASE("AddGate") {

  std::size_t threshold = 4; // has to be even
  std::size_t batch_size = (threshold + 2)/2;
  std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
  
  SECTION("AddGate") {
    std::vector<std::shared_ptr<tp::InputGate>> x_gates;
    std::vector<std::shared_ptr<tp::InputGate>> y_gates;
    x_gates.reserve(n_parties);
    y_gates.reserve(n_parties);

    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates.emplace_back(std::make_shared<tp::InputGate>(0));
      y_gates.emplace_back(std::make_shared<tp::InputGate>(0));
    }

    tp::FF X(21321);
    tp::FF Y(-3421);

    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates[i]->_SetDummyInput(X);
      y_gates[i]->_SetDummyInput(Y);
    }

    std::vector<std::shared_ptr<tp::AddGate>> z_gates;
    z_gates.reserve(n_parties);

    for (std::size_t i = 0; i < n_parties; i++) {
      z_gates.emplace_back(std::make_shared<tp::AddGate>( x_gates[i], y_gates[i] ));
    }

    REQUIRE(!z_gates[0]->IsLearned()); // P1 hasn't learned anything yet

    tp::FF Z = x_gates[0]->GetMu() + y_gates[0]->GetMu();
    REQUIRE( Z == z_gates[0]->GetMu() ); 

    REQUIRE(z_gates[0]->IsLearned()); // P1 learned mu already
    REQUIRE(!z_gates[1]->IsLearned()); // But not other parties
  }
}
