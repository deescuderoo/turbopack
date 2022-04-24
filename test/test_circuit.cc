#include <catch2/catch.hpp>
#include <iostream>

#include "tp/circuits.h"

#define PARTY for(std::size_t i = 0; i < n_parties; i++)

TEST_CASE("Cleartext") {
  SECTION("Hand-crafted circuit") {
    // Inputs x,y,u,v
    // x' = (x+y)*x,  y' = (x+y)*y
    // u' = (u+v)*u,  v' = (u+v)*v
    // z = (x' + y') + (u' + v')

    std::size_t batch_size(1);
    std::size_t n_clients(2);
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

    // Evaluate
    tp::FF X(21321);
    tp::FF Y(-3421);
    tp::FF U(170942);
    tp::FF V(-894);

    x->ClearInput(X);
    y->ClearInput(Y);
    u->ClearInput(U);
    v->ClearInput(V);

    // Check that the addition gates are not populated yet
    REQUIRE(!(xPy->IsEvaluated()));
    REQUIRE(!(uPv->IsEvaluated()));

    // Testing GetClear()
    // std::vector<tp::FF> inputs{X, U, Y, V}; // watch out for the ordering!
    // c.SetClearInputsFlat(inputs);
    auto result = c.GetClearOutputsFlat()[0];

    // Check that the call above populates upper gates
    REQUIRE(xPy->IsEvaluated());
    REQUIRE(uPv->IsEvaluated());

    // Check output
    tp::FF real = (X+Y)*X + (X+Y)*Y + (U+V)*U + (U+V)*V;
    REQUIRE(result == real);
  }

  SECTION("Artificial circuit") 
    {
      std::size_t batch_size = 8;
      std::size_t n_parties = 4*batch_size - 3;

      tp::CircuitConfig config;
      config.n_parties = n_parties;
      config.inp_gates = std::vector<std::size_t>(n_parties, 0);
      config.inp_gates[0] = 2;
      config.out_gates = std::vector<std::size_t>(n_parties, 0);
      config.out_gates[0] = 2;
      config.width = 100;
      config.depth = 2;
      config.batch_size = batch_size;
	  
      auto circuit = tp::Circuit::FromConfig(config);

      // Test metrics
      REQUIRE(circuit.GetDepth() == config.depth);
      REQUIRE(circuit.GetNInputs() == config.ComputeNumberOfInputs());
      REQUIRE(circuit.GetNOutputs() == config.ComputeNumberOfOutputs());
      REQUIRE(circuit.GetWidth() == config.width);
      REQUIRE(circuit.GetSize() == config.width * config.depth);

      // Test batch_size does not affect result

      config.batch_size = 6;
      auto circuit_2 = tp::Circuit::FromConfig(config);

      std::vector<tp::FF> inputs{tp::FF(-83427), tp::FF(185732)};
      circuit.SetClearInputsFlat(inputs);
      auto result = circuit.GetClearOutputsFlat()[0];
      circuit_2.SetClearInputsFlat(inputs);
      auto result_2 = circuit_2.GetClearOutputsFlat()[0];

      REQUIRE(result == result_2);

      // TEST NON-FLAT INPUTS/OUTPUTS
      tp::CircuitConfig config_;
      config_.n_parties = 3;
      config_.inp_gates = {1,1,0};
      config_.out_gates = {1,1,0};
      config_.width = 100;
      config_.depth = 2;
      config_.batch_size = 2;
	  
      auto circuit_3 = tp::Circuit::FromConfig(config_);

      // Test metrics
      REQUIRE(circuit_3.GetDepth() == config_.depth);
      REQUIRE(circuit_3.GetNInputs() == config_.ComputeNumberOfInputs());
      REQUIRE(circuit_3.GetNOutputs() == config_.ComputeNumberOfOutputs());
      REQUIRE(circuit_3.GetWidth() == config_.width);
      REQUIRE(circuit_3.GetSize() == config_.width * config_.depth);

      // Test batch_size does not affect result

      config_.batch_size = 8;
      auto circuit_4 = tp::Circuit::FromConfig(config_);

      std::vector<std::vector<tp::FF>> inputs_(3);
      inputs_[0] = std::vector<tp::FF>{tp::FF(0)};
      inputs_[1] = std::vector<tp::FF>{tp::FF(1)};

      circuit_3.SetClearInputs(inputs_);
      auto result_3a = circuit_3.GetClearOutputs()[0][0];
      auto result_3b = circuit_3.GetClearOutputs()[1][0];
      circuit_4.SetClearInputs(inputs_);
      auto result_4a = circuit_4.GetClearOutputs()[0][0];
      auto result_4b = circuit_4.GetClearOutputs()[1][0];

      REQUIRE(result_3a == result_4a);
      REQUIRE(result_3b == result_4b);
    }  
}

TEST_CASE("Secure computation") {
  SECTION("Hand-crafted circuit") {
    // Inputs x,y,u,v
    // x' = (x+y)*x,  y' = (x+y)*y
    // u' = (u+v)*u,  v' = (u+v)*v
    // z = (x' + y') + (u' + v')

    std::size_t threshold = 4; // has to be even
    std::size_t batch_size = (threshold + 2)/2;
    std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
    auto networks = scl::Network::CreateFullInMemory(n_parties);
    std::size_t n_clients = n_parties;

    tp::FF lambda(649823289);

    std::vector<tp::Circuit> circuits;
    circuits.reserve(n_parties);

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
      c._DummyPrep(lambda);

      circuits.emplace_back(c);
    }

    // SET INPUTS
    tp::FF X(21321);
    tp::FF Y(-3421);
    tp::FF U(170942);
    tp::FF V(-894);

    // P1 sets inpus X and U
    circuits[0].SetInputs(std::vector<tp::FF>{X, U});
    // P2 sets inpus Y and V
    circuits[1].SetInputs(std::vector<tp::FF>{Y, V});

    // Input protocol
    REQUIRE(circuits[0].GetInputGate(0,0)->IsLearned() == false);
    REQUIRE(circuits[0].GetInputGate(0,1)->IsLearned() == false);
    REQUIRE(circuits[0].GetInputGate(1,0)->IsLearned() == false);
    REQUIRE(circuits[0].GetInputGate(1,1)->IsLearned() == false);

    PARTY { circuits[i].InputOwnerSendsP1(); }
    PARTY { circuits[i].InputP1Receives(); }

    REQUIRE(circuits[0].GetInputGate(0,0)->GetMu() == X - lambda);
    REQUIRE(circuits[0].GetInputGate(0,1)->GetMu() == U - lambda);
    REQUIRE(circuits[0].GetInputGate(1,0)->GetMu() == Y - lambda);
    REQUIRE(circuits[0].GetInputGate(1,1)->GetMu() == V - lambda);

    // Multiplications (there is only one layer)
    REQUIRE(circuits[0].GetMultGate(0,0)->IsLearned() == false);
    REQUIRE(circuits[0].GetMultGate(0,1)->IsLearned() == false);
    REQUIRE(circuits[0].GetMultGate(0,2)->IsLearned() == false);
    REQUIRE(circuits[0].GetMultGate(0,3)->IsLearned() == false);
    
    PARTY { circuits[i].MultP1Sends(0); }
    PARTY { circuits[i].MultPartiesReceive(0); }
    PARTY { circuits[i].MultPartiesSend(0); }
    PARTY { circuits[i].MultP1Receives(0); }

    REQUIRE(circuits[0].GetMultGate(0,0)->GetMu() == (X+Y)*X - lambda);
    REQUIRE(circuits[0].GetMultGate(0,1)->GetMu() == (X+Y)*Y - lambda);
    REQUIRE(circuits[0].GetMultGate(0,2)->GetMu() == (U+V)*U - lambda);
    REQUIRE(circuits[0].GetMultGate(0,3)->GetMu() == (U+V)*V - lambda);
    
    // Output protocol
    PARTY { circuits[i].OutputP1SendsMu(); }
    PARTY { circuits[i].OutputOwnerReceivesMu(); }

    // Check output
    tp::FF real = (X+Y)*X + (X+Y)*Y + (U+V)*U + (U+V)*V;
    REQUIRE(circuits[0].GetOutputGate(0,0)->GetValue() == real);
  }
  
  SECTION("Artificial circuit") 
    {
      std::size_t batch_size = 2;
      std::size_t n_parties = 4*batch_size - 3;

      tp::CircuitConfig config;
      config.n_parties = n_parties;
      config.inp_gates = std::vector<std::size_t>(n_parties, 0);
      config.inp_gates[0] = 2;
      config.out_gates = std::vector<std::size_t>(n_parties, 0);
      config.out_gates[0] = 2;
      config.width = 100;
      config.depth = 2;
      config.batch_size = batch_size;
	  
      auto networks = scl::Network::CreateFullInMemory(n_parties);

      std::vector<tp::Circuit> circuits;
      circuits.reserve(n_parties);

      tp::FF lambda(-45298432);

      PARTY {
	auto c = tp::Circuit::FromConfig(config);
	c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);
	c._DummyPrep(lambda);
	circuits.emplace_back(c);
      }

      std::vector<tp::FF> inputs{tp::FF(0432432), tp::FF(54982)};
      circuits[0].SetClearInputsFlat(inputs);
      auto result = circuits[0].GetClearOutputsFlat();
      circuits[0].SetInputs(inputs);

      // INPUT
      PARTY { circuits[i].InputOwnerSendsP1(); }
      PARTY { circuits[i].InputP1Receives(); }

      // MULT
      for (std::size_t layer = 0; layer < config.depth; layer++) {
	PARTY { circuits[i].MultP1Sends(layer); }
	PARTY { circuits[i].MultPartiesReceive(layer); }
	PARTY { circuits[i].MultPartiesSend(layer); }
	PARTY { circuits[i].MultP1Receives(layer); }
      }

      // OUTPUT
      PARTY { circuits[i].OutputP1SendsMu(); }
      PARTY { circuits[i].OutputOwnerReceivesMu(); }

      // Check output
      REQUIRE(circuits[0].GetOutputs() == result);
    }
}
