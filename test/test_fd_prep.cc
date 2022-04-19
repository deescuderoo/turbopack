#include <catch2/catch.hpp>
#include <iostream>

#include "tp/circuits.h"

#define PARTY for(std::size_t i = 0; i < n_parties; i++)

TEST_CASE("Dummy FD") {
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

      c._DummyPrep(lambda);
      // TODO remove these functions below from Circuit. Not needed anymore
      // c.SetDummyLambdas(lambda);
      // c.PopulateDummyLambdas();
      // c.PrepFromDummyLambdas();

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

TEST_CASE("FD from Dummy FI") {
  SECTION("Shares of canonical vectors"){
    std::size_t dim(10);
    std::vector<tp::Vec> shares_ei;
    shares_ei.resize(dim);
    for (std::size_t i = 0; i < dim; i++) { shares_ei[i].Reserve(dim); }

    // Create shares
    for (std::size_t shr_id = 0; shr_id < dim; shr_id++) {
      for (std::size_t idx = 0; idx < dim; idx++) {
	tp::FF shr(1);
	for (std::size_t j = 0; j < dim; ++j) {
	  if (j == idx) continue;
	  shr *= (tp::FF(shr_id+1) + tp::FF(j)) / (tp::FF(j) - tp::FF(idx));
	}
	shares_ei[idx].Emplace(shr);
      }
    }

    // Determine corresponding secrets
    std::vector<tp::Vec> secrets_ei;
    secrets_ei.resize(dim);
    for (std::size_t i = 0; i < dim; i++) {
      secrets_ei[i] = scl::details::SecretsFromSharesAndLength(shares_ei[i], dim);
    }

    // Test each e_i
    for (std::size_t i = 0; i < dim; i++) {
      for (std::size_t j = 0; j < dim; j++) {
	if (i==j) REQUIRE( secrets_ei[i][j] == tp::FF(1) );
	if (i!=j) REQUIRE( secrets_ei[i][j] == tp::FF(0) );
      }
    }
  }

  SECTION("Hand-made Circuit")     {
    std::size_t threshold = 4; // has to be even
    std::size_t batch_size = (threshold + 2)/2;
    std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
    auto networks = scl::Network::CreateFullInMemory(n_parties);
    std::size_t n_clients = n_parties;

    tp::FF lambda(-5342891);

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

      c.GenCorrelator();
      c.PopulateDummyCorrelator(lambda);
      c.MapCorrToCircuit();

      circuits.emplace_back(c);
    }

    // INPUT+OUTPUT+MULT 
	 PARTY { circuits[i].PrepMultPartiesSendP1(); }
    PARTY { circuits[i].PrepMultP1ReceivesAndSends(); }
    PARTY { circuits[i].PrepMultPartiesReceive(); }
    PARTY { circuits[i].PrepIOPartiesSendOwner(); }
    PARTY { circuits[i].PrepIOOwnerReceives(); }

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

  SECTION("Generic Circuit")     {
    std::size_t batch_size = 10;
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

    tp::FF lambda(23413);

    PARTY {
      auto c = tp::Circuit::FromConfig(config);
      c.SetNetwork(std::make_shared<scl::Network>(networks[i]), i);

      c.GenCorrelator();
      c.PopulateDummyCorrelator(lambda);
      c.MapCorrToCircuit();

      circuits.emplace_back(c);
    }

    // INPUT+OUTPUT+MULT 
	 PARTY { circuits[i].PrepMultPartiesSendP1(); }
    PARTY { circuits[i].PrepMultP1ReceivesAndSends(); }
    PARTY { circuits[i].PrepMultPartiesReceive(); }
    // Can be run inline with the above
    PARTY { circuits[i].PrepIOPartiesSendOwner(); }
    PARTY { circuits[i].PrepIOOwnerReceives(); }


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

TEST_CASE("FD from Real FI") {
  SECTION("Hand-made Circuit")     {
    std::size_t threshold = 4; // has to be even
    std::size_t batch_size = (threshold + 2)/2;
    std::size_t n_parties = threshold + 2*(batch_size - 1) + 1;
    auto networks = scl::Network::CreateFullInMemory(n_parties);
    std::size_t n_clients = n_parties;

    tp::FF lambda(-5342891);

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

      c.GenCorrelator();
      c.SetThreshold(threshold);

      circuits.emplace_back(c);
    }

    PARTY {
      circuits[i].GenIndShrsPartiesSend();
      circuits[i].GenUnpackedShrPartiesSend();
      circuits[i].GenZeroPartiesSend(); 
      circuits[i].GenZeroForProdPartiesSend();
    }

    PARTY {
      circuits[i].GenIndShrsPartiesReceive();
      circuits[i].GenUnpackedShrPartiesReceive();
      circuits[i].GenZeroPartiesReceive(); 
      circuits[i].GenZeroForProdPartiesReceive();
    }

    // // Test ind_shrs
    // for ( std::size_t j = 0; j < circuits[0].GetCorrelator().mIndShrs.size(); j++ ) {
    //   tp::Vec shares;
    //   for ( std::size_t i = 0; i < n_parties - batch_size + 1; i++ ) shares.Emplace(circuits[i].GetCorrelator().mIndShrs[j]);
    //   auto secrets = scl::details::SecretsFromSharesAndLength(shares, batch_size);
    //   for ( std::size_t i = 0; i < batch_size; i++ ) REQUIRE( secrets[i] == secrets[0] );
    // }

    // // Test zero shrs
    // for ( std::size_t j = 0; j < circuits[0].GetCorrelator().mIOBatchFIPrep.size(); j++ ) {
    //   tp::Vec shares;
    //   for ( std::size_t i = 0; i < n_parties; i++ ) shares.Emplace(circuits[i].GetCorrelator().mIOBatchFIPrep[j].mShrO);
    //   auto secret = scl::details::SecretFromShares(shares);
    //   REQUIRE( secret == tp::FF(0) );
    // }

    // // Test zero shrs
    // for ( std::size_t k = 0; k < circuits[0].GetCorrelator().mZeroProdShrs.size(); k++ ) {
    
    //   for ( std::size_t j = 0; j < circuits[0].GetCorrelator().mZeroProdShrs[k].size(); j++ ) {
    // 	tp::Vec shares;
    // 	for ( std::size_t i = 0; i < n_parties; i++ ) shares.Emplace(circuits[i].GetCorrelator().mZeroProdShrs[k][j]);
    // 	auto secret = scl::details::SecretFromPointAndShares(tp::FF(-k), shares);
    // 	REQUIRE( secret == tp::FF(0) );
    //   }
    // }

    PARTY { circuits[i].GenProdPartiesSendP1(); }
    PARTY { circuits[i].GenProdP1ReceivesAndSends(); }
    PARTY { circuits[i].GenProdPartiesReceive(); }
  
    // // Test mult
    // for ( std::size_t j = 0; j < circuits[0].GetCorrelator().mMultBatchFIPrep.size(); j++ ) {
    //   tp::Vec sharesA;
    //   tp::Vec sharesB;
    //   tp::Vec sharesC;
    //   for ( std::size_t i = 0; i < n_parties - batch_size + 1; i++ ) {
    // 	sharesA.Emplace(circuits[i].GetCorrelator().mMultBatchFIPrep[j].mShrA);
    // 	sharesB.Emplace(circuits[i].GetCorrelator().mMultBatchFIPrep[j].mShrB);
    // 	sharesC.Emplace(circuits[i].GetCorrelator().mMultBatchFIPrep[j].mShrC);
    //   }
    //   auto secretsA = scl::details::SecretsFromSharesAndLength(sharesA, batch_size);
    //   auto secretsB = scl::details::SecretsFromSharesAndLength(sharesB, batch_size);
    //   auto secretsC = scl::details::SecretsFromSharesAndLength(sharesC, batch_size);

    //   for ( std::size_t i = 0; i < batch_size; i++ ) {
    // 	REQUIRE( secretsA[i] * secretsB[i] == secretsC[i] );
    //   }
    // }

    PARTY { circuits[i].MapCorrToCircuit(); }

    // INPUT+OUTPUT+MULT 
	 PARTY { circuits[i].PrepMultPartiesSendP1(); }
    PARTY { circuits[i].PrepMultP1ReceivesAndSends(); }
    PARTY { circuits[i].PrepMultPartiesReceive(); }
    PARTY { circuits[i].PrepIOPartiesSendOwner(); }
    PARTY { circuits[i].PrepIOOwnerReceives(); }

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

    // Multiplications (there is only one layer)
    REQUIRE(circuits[0].GetMultGate(0,0)->IsLearned() == false);
    REQUIRE(circuits[0].GetMultGate(0,1)->IsLearned() == false);
    REQUIRE(circuits[0].GetMultGate(0,2)->IsLearned() == false);
    REQUIRE(circuits[0].GetMultGate(0,3)->IsLearned() == false);
    
    PARTY { circuits[i].MultP1Sends(0); }
    PARTY { circuits[i].MultPartiesReceive(0); }
    PARTY { circuits[i].MultPartiesSend(0); }
    PARTY { circuits[i].MultP1Receives(0); }
    
    // Output protocol
    PARTY { circuits[i].OutputP1SendsMu(); }
    PARTY { circuits[i].OutputOwnerReceivesMu(); }

    // Check output
    tp::FF real = (X+Y)*X + (X+Y)*Y + (U+V)*U + (U+V)*V;
    REQUIRE(circuits[0].GetOutputGate(0,0)->GetValue() == real);
  }
}
