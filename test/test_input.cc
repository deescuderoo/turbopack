#include <catch2/catch.hpp>
#include <iostream>

#include "tp/input_gate.h"

TEST_CASE("InputGate") {
  std::size_t threshold = 4; // has to be even
  std::size_t batch_size = (threshold + 2)/2;
  std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
  auto networks = scl::Network::CreateFullInMemory(n_parties);

  SECTION("InputGate") {
    std::vector<std::shared_ptr<tp::InputGate>> x_gates;
    std::vector<std::shared_ptr<tp::InputGate>> y_gates;
    x_gates.reserve(n_parties);
    y_gates.reserve(n_parties);

    tp::FF lambda_x(50942);
    tp::FF lambda_y(-4523);

    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates.emplace_back(std::make_shared<tp::InputGate>(0)); // P1 owner
      x_gates[i]->SetNetwork(networks[i], i);
      x_gates[i]->_DummyPrep(lambda_x);

      y_gates.emplace_back(std::make_shared<tp::InputGate>(1)); // P2 owner
      y_gates[i]->SetNetwork(networks[i], i);
      y_gates[i]->_DummyPrep(lambda_y);
    }

    tp::FF X(542);
    tp::FF Y(-45);

    // Set inputs
    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates[i]->SetInput(X);
      y_gates[i]->SetInput(Y);
    }

    // Send to P1
    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates[i]->OwnerSendsP1();
      y_gates[i]->OwnerSendsP1();
    }

    REQUIRE(!x_gates[0]->IsLearned());
    REQUIRE(!y_gates[0]->IsLearned());

    // P1 receives
    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates[i]->P1Receives();
      y_gates[i]->P1Receives();
    }

    REQUIRE(x_gates[0]->IsLearned());
    REQUIRE(y_gates[0]->IsLearned());

    tp::FF mu_x = x_gates[0]->GetMu();
    tp::FF mu_y = y_gates[0]->GetMu();
    REQUIRE(X == mu_x + lambda_x);
    REQUIRE(Y == mu_y + lambda_y);
  }

}
