#include <catch2/catch.hpp>
#include <iostream>

#include "tp/circuits.h"

#define PARTY for(std::size_t i = 0; i < n_parties; i++)

TEST_CASE("Dummy preprocessing") {
  SECTION("Correct result")    {
    tp::CircuitConfig config;
    config.n_parties = 3;
    config.inp_gates = {2,0,0};
    config.out_gates = {2,0,0};
    config.width = 100;
    config.depth = 2;
    config.batch_size = 2;
	  
    std::size_t n_parties = config.n_parties;
    auto networks = scl::Network::CreateFullInMemory(n_parties);

    tp::FF lambda(28);

    std::vector<tp::Circuit> circuits;
    circuits.reserve(n_parties);

    PARTY {
      auto c = tp::Circuit::FromConfig(config);
      c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);
      // c._DummyPrep();

      c.SetDummyLambdas(lambda);
      c.PopulateDummyLambdas();
      c.PrepFromDummyLambdas();

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
