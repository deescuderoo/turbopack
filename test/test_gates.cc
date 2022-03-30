#include <catch2/catch.hpp>
#include <iostream>

#include "tp/circuits.h"

std::size_t threshold = 4; // has to be even
std::size_t batch_size = (threshold + 2)/2;
std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
auto networks = scl::Network::CreateFullInMemory(n_parties);

TEST_CASE("Gates") {
  SECTION("InputGate") {
    std::vector<std::shared_ptr<tp::InputGate>> x_gates;
    std::vector<std::shared_ptr<tp::InputGate>> y_gates;
    x_gates.reserve(n_parties);
    y_gates.reserve(n_parties);

    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates.emplace_back(std::make_shared<tp::InputGate>(0)); // P1 owner
      x_gates[i]->SetNetwork(networks[i], i);

      y_gates.emplace_back(std::make_shared<tp::InputGate>(1)); // P2 owner
      y_gates[i]->SetNetwork(networks[i], i);
    }

    tp::FF X(542);
    tp::FF Y(-45);

    // Send to P1
    for (std::size_t i = 0; i < n_parties; i++) {
      x_gates[i]->OwnerSendsP1(X);
      y_gates[i]->OwnerSendsP1(Y);
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

    // TODO this will break once we get actual preprocessing
    REQUIRE(x_gates[0]->GetMu() == X);
    REQUIRE(y_gates[0]->GetMu() == Y);
  }

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
  
  SECTION("MultGate") {
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

    // This works because the lambda masks are all zero
    for (std::size_t i = 0; i < n_parties; i++) {
      REQUIRE( X == x_gates[i]->GetMu() );
      REQUIRE( Y== y_gates[i]->GetMu() );
    }   

    std::vector<std::shared_ptr<tp::MultGate>> z_gates;
    z_gates.reserve(n_parties);

    for (std::size_t i = 0; i < n_parties; i++) {
      z_gates.emplace_back(std::make_shared<tp::MultGate>( x_gates[i], y_gates[i] ));
    }

    // Setting batches
    std::vector<std::shared_ptr<tp::MultBatch>> batches;
    batches.reserve(n_parties);

    for (std::size_t i = 0; i < n_parties; i++) {
      batches.emplace_back(std::make_shared<tp::MultBatch>(1));
      batches[i]->Append(z_gates[i]);
      batches[i]->SetNetwork(networks[i], i);
    }

    for (std::size_t i = 0; i < n_parties; i++) {
      batches[i]->P1Sends(); // Only P1 works here
    }
    
    for (std::size_t i = 0; i < n_parties; i++) {
      batches[i]->PartiesReceive(); 
    }

    for (std::size_t i = 0; i < n_parties; i++) {
      batches[i]->PartiesSend(); 
    }

    REQUIRE(!z_gates[0]->IsLearned()); // P1 hasn't learned anything yet

    for (std::size_t i = 0; i < n_parties; i++) {
      batches[i]->P1Receives(); // Only P1 works here
    }

    REQUIRE(z_gates[0]->IsLearned()); // P1 learned mu already
    REQUIRE(!z_gates[1]->IsLearned()); // But not other parties

    // Check result
    
    tp::FF Z = X*Y;
    REQUIRE( Z == z_gates[0]->GetMu() ); // TODO this breaks with actual preprocessing
  }

}