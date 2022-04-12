#include <catch2/catch.hpp>
#include <iostream>

#include "tp/circuits.h"

TEST_CASE("Circuits") {
  SECTION("Clear circuit") {
    // Inputs x,y,u,v
    // x' = (x+y)*x,  y' = (x+y)*y
    // u' = (u+v)*u,  v' = (u+v)*v
    // z = (x' + y') + (u' + v')

    auto c = tp::Circuit();
    
    auto x = c.Input(0);
    auto y = c.Input(0);
    auto u = c.Input(0);
    auto v = c.Input(0);

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
    std::vector<tp::FF> inputs{X, Y, U, V};
    c.SetClearInputsFlat(inputs);
    auto result = c.GetClearOutputsFlat()[0];

    // Check that the call above populates upper gates
    REQUIRE(xPy->IsEvaluated());
    REQUIRE(uPv->IsEvaluated());

    // Check output
    tp::FF real = (X+Y)*X + (X+Y)*Y + (U+V)*U + (U+V)*V;
    REQUIRE(result == real);
  }
  
  SECTION("Artificial Circuit") 
    {
      tp::CircuitConfig config;
      config.n_parties = 3;
      config.inp_gates = {2,0,0};
      config.out_gates = {2,0,0};
      config.width = 100;
      config.depth = 2;
      config.batch_size = 2;
	  
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

      std::vector<tp::FF> inputs{tp::FF(0), tp::FF(1)};
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
