#include <catch2/catch.hpp>
#include <iostream>

#include "tp/dn07.h"

#define PARTY for(std::size_t i = 0; i < n_parties; i++)

TEST_CASE("DN07: Dummy FD") {
  SECTION("Hand-made Circuit")     {
    std::size_t threshold = 4; // has to be even
    std::size_t batch_size = (threshold + 2)/2;
    std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
    auto networks = scl::Network::CreateFullInMemory(n_parties);
    std::size_t n_clients = n_parties;

    tp::FF prep(-5342891);

    std::vector<tp::Circuit> circuits;
    circuits.reserve(n_parties);
    std::vector<tp::DN07> dn07es;
    dn07es.reserve(n_parties);

    PARTY {
      auto c = tp::Circuit(n_clients, batch_size);
    
      auto x = c.Input(0);
      auto y = c.Input(1);
      auto u = c.Input(0);
      auto v = c.Input(1);

      c.CloseInputs();
    
      auto xPy = c.Add(x, y);
      auto uPv = c.Add(u, v);

      auto x_ = c.Mult(xPy, x);
      auto y_ = c.Mult(xPy, y);
      auto u_ = c.Mult(uPv, u);
      auto v_ = c.Mult(uPv, v);
      c.LastLayer();

   
      auto z1 = c.Add(x_, y_);
      auto z2 = c.Add(u_, v_);

      auto z = c.Add(z1, z2);

      auto output = c.Output(0,z);
      c.CloseOutputs();

      c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);


      tp::DN07 dn07(n_parties, threshold);
      dn07.SetCircuit(c);
   
      dn07es.emplace_back(dn07);
    }

    // DUMMY PREP
    PARTY { dn07es[i].DummyPrep(prep); }

    // FD PREP
    PARTY { dn07es[i].FDMapPrepToGates(); }
    PARTY { dn07es[i].FDMultPartiesSendP1(); }
    PARTY { dn07es[i].FDMultP1Receives(); }

    // SET INPUTS
    tp::FF X(21321);
    tp::FF Y(-3421);
    tp::FF U(170942);
    tp::FF V(-894);

    // P1 sets inpus X and U
    dn07es[0].GetCircuit().SetInputs(std::vector<tp::FF>{X, U});
    // P2 sets inpus Y and V
    dn07es[1].GetCircuit().SetInputs(std::vector<tp::FF>{Y, V});

    // Input protocol

    PARTY { dn07es[i].InputPartiesSendOwners(); }
    PARTY { dn07es[i].InputOwnersReceiveAndSendParties(); }
    PARTY { dn07es[i].InputPartiesReceive(); }

    // Multiplications (there is only one layer)
    
    PARTY { dn07es[i].MultPartiesSendP1(0); }
    PARTY { dn07es[i].MultP1ReceivesAndSendsParties(0); }
    PARTY { dn07es[i].MultPartiesReceive(0); }

    // Output protocol
    PARTY { dn07es[i].OutputPartiesSendOwners(); }
    PARTY { dn07es[i].OutputOwnersReceive(); }

    // Check output
    tp::FF real = (X+Y)*X + (X+Y)*Y + (U+V)*U + (U+V)*V;
    REQUIRE(dn07es[0].GetOutput(0,0) == real);
  }
  
  SECTION("Generic Circuit")     {
    std::size_t threshold = 6; // has to be even
    std::size_t batch_size = (threshold + 2)/2;
    std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;

    tp::CircuitConfig config;
    config.n_parties = n_parties;
    config.inp_gates = std::vector<std::size_t>(n_parties, 0);
    config.inp_gates[0] = 2;
    config.out_gates = std::vector<std::size_t>(n_parties, 0);
    config.out_gates[0] = 2;
    config.width = 100;
    config.depth = 3;
    config.batch_size = batch_size;

    auto networks = scl::Network::CreateFullInMemory(n_parties);

    tp::FF prep(-5342891);

    std::vector<tp::DN07> dn07es;
    dn07es.reserve(n_parties);

    PARTY {
      auto c = tp::Circuit::FromConfig(config);
      c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);

      c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);

      tp::DN07 dn07(n_parties, threshold);
      dn07.SetCircuit(c);
   
      dn07es.emplace_back(dn07);
    }

    // DUMMY PREP
    PARTY { dn07es[i].DummyPrep(prep); }

    // FD PREP
    PARTY { dn07es[i].FDMapPrepToGates(); }
    PARTY { dn07es[i].FDMultPartiesSendP1(); }
    PARTY { dn07es[i].FDMultP1Receives(); }

    // Set inputs
    std::vector<tp::FF> inputs{tp::FF(0432432), tp::FF(54982)};
    dn07es[0].GetCircuit().SetClearInputsFlat(inputs);
    auto result = dn07es[0].GetCircuit().GetClearOutputsFlat();
    dn07es[0].GetCircuit().SetInputs(inputs);

    // Input protocol

    PARTY { dn07es[i].InputPartiesSendOwners(); }
    PARTY { dn07es[i].InputOwnersReceiveAndSendParties(); }
    PARTY { dn07es[i].InputPartiesReceive(); }

    // Multiplications (there is only one layer)
    
    for (std::size_t layer = 0; layer < config.depth; layer++) {
      PARTY { dn07es[i].MultPartiesSendP1(layer); }
      PARTY { dn07es[i].MultP1ReceivesAndSendsParties(layer); }
      PARTY { dn07es[i].MultPartiesReceive(layer); }
    }
    // Output protocol
    PARTY { dn07es[i].OutputPartiesSendOwners(); }
    PARTY { dn07es[i].OutputOwnersReceive(); }

    // Check output
    REQUIRE(dn07es[0].GetOutput(0,0) == result[0]);
  }  
}

TEST_CASE("DN07: Real Prep") {
  SECTION("Hand-made Circuit")     {
    std::size_t threshold = 4; // has to be even
    std::size_t batch_size = (threshold + 2)/2;
    std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
    auto networks = scl::Network::CreateFullInMemory(n_parties);
    std::size_t n_clients = n_parties;

    std::vector<tp::Circuit> circuits;
    circuits.reserve(n_parties);
    std::vector<tp::DN07> dn07es;
    dn07es.reserve(n_parties);

    PARTY {
      auto c = tp::Circuit(n_clients, batch_size);
    
      auto x = c.Input(0);
      auto y = c.Input(1);
      auto u = c.Input(0);
      auto v = c.Input(1);

      c.CloseInputs();
    
      auto xPy = c.Add(x, y);
      auto uPv = c.Add(u, v);

      auto x_ = c.Mult(xPy, x);
      auto y_ = c.Mult(xPy, y);
      auto u_ = c.Mult(uPv, u);
      auto v_ = c.Mult(uPv, v);
      c.LastLayer();

   
      auto z1 = c.Add(x_, y_);
      auto z2 = c.Add(u_, v_);

      auto z = c.Add(z1, z2);

      auto output = c.Output(0,z);
      c.CloseOutputs();

      c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);


      tp::DN07 dn07(n_parties, threshold);
      dn07.SetCircuit(c);
   
      dn07es.emplace_back(dn07);
    }

    // Prep
    PARTY { dn07es[i].PrepPartiesSend(); }
    PARTY { dn07es[i].PrepPartiesReceive(); }

    // FD PREP
    PARTY { dn07es[i].FDMapPrepToGates(); }
    PARTY { dn07es[i].FDMultPartiesSendP1(); }
    PARTY { dn07es[i].FDMultP1Receives(); }

    // SET INPUTS
    tp::FF X(12);
    tp::FF Y(24);
    tp::FF U(36);
    tp::FF V(48);

    // P1 sets inpus X and U
    dn07es[0].GetCircuit().SetInputs(std::vector<tp::FF>{X, U});
    // P2 sets inpus Y and V
    dn07es[1].GetCircuit().SetInputs(std::vector<tp::FF>{Y, V});

    // Input protocol

    PARTY { dn07es[i].InputPartiesSendOwners(); }
    PARTY { dn07es[i].InputOwnersReceiveAndSendParties(); }
    PARTY { dn07es[i].InputPartiesReceive(); }

    // Multiplications (there is only one layer)
    
    PARTY { dn07es[i].MultPartiesSendP1(0); }
    PARTY { dn07es[i].MultP1ReceivesAndSendsParties(0); }
    PARTY { dn07es[i].MultPartiesReceive(0); }

    // Output protocol
    PARTY { dn07es[i].OutputPartiesSendOwners(); }
    PARTY { dn07es[i].OutputOwnersReceive(); }

    // Check output
    tp::FF real = (X+Y)*X + (X+Y)*Y + (U+V)*U + (U+V)*V;
    REQUIRE(dn07es[0].GetOutput(0,0) == real);
  }

  SECTION("Generic Circuit")     {
    std::size_t threshold = 6; // has to be even
    std::size_t batch_size = (threshold + 2)/2;
    std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;

    tp::CircuitConfig config;
    config.n_parties = n_parties;
    config.inp_gates = std::vector<std::size_t>(n_parties, 0);
    config.inp_gates[0] = 2;
    config.out_gates = std::vector<std::size_t>(n_parties, 0);
    config.out_gates[0] = 2;
    config.width = 100;
    config.depth = 3;
    config.batch_size = batch_size;

    auto networks = scl::Network::CreateFullInMemory(n_parties);

    tp::FF prep(-5342891);

    std::vector<tp::DN07> dn07es;
    dn07es.reserve(n_parties);

    PARTY {
      auto c = tp::Circuit::FromConfig(config);
      c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);

      c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);

      tp::DN07 dn07(n_parties, threshold);
      dn07.SetCircuit(c);
   
      dn07es.emplace_back(dn07);
    }

    // Prep
    PARTY { dn07es[i].PrepPartiesSend(); }
    PARTY { dn07es[i].PrepPartiesReceive(); }

    // FD PREP
    PARTY { dn07es[i].FDMapPrepToGates(); }
    PARTY { dn07es[i].FDMultPartiesSendP1(); }
    PARTY { dn07es[i].FDMultP1Receives(); }

    // Set inputs
    std::vector<tp::FF> inputs{tp::FF(0432432), tp::FF(54982)};
    dn07es[0].GetCircuit().SetClearInputsFlat(inputs);
    auto result = dn07es[0].GetCircuit().GetClearOutputsFlat();
    dn07es[0].GetCircuit().SetInputs(inputs);

    // Input protocol

    PARTY { dn07es[i].InputPartiesSendOwners(); }
    PARTY { dn07es[i].InputOwnersReceiveAndSendParties(); }
    PARTY { dn07es[i].InputPartiesReceive(); }

    // Multiplications (there is only one layer)
    
    for (std::size_t layer = 0; layer < config.depth; layer++) {
      PARTY { dn07es[i].MultPartiesSendP1(layer); }
      PARTY { dn07es[i].MultP1ReceivesAndSendsParties(layer); }
      PARTY { dn07es[i].MultPartiesReceive(layer); }
    }
    // Output protocol
    PARTY { dn07es[i].OutputPartiesSendOwners(); }
    PARTY { dn07es[i].OutputOwnersReceive(); }

    // Check output
    REQUIRE(dn07es[0].GetOutput(0,0) == result[0]);
  }
}

