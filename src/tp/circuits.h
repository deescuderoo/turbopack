#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <catch2/catch.hpp>
#include <iostream>
#include <assert.h>

#include "tp/mult_gate.h"
#include "tp/input_gate.h"
#include "tp/output_gate.h"

#include "tp/correlator.h"

using VecMultGates = std::vector<std::shared_ptr<tp::MultGate>>;

namespace tp {
  struct CircuitConfig {
    std::vector<std::size_t> inp_gates;
    std::vector<std::size_t> out_gates;
    std::size_t n_parties;
    std::size_t width;
    std::size_t depth;
    std::size_t batch_size;

    std::size_t ComputeNumberOfInputs() {
      std::size_t sum(0);
      for (auto n_inputs : inp_gates)
	sum += n_inputs;
      return sum;
    }
    std::size_t ComputeNumberOfOutputs() {
      std::size_t sum(0);
      for (auto n_outputs : out_gates)
	sum += n_outputs;
      return sum;
    }
  };

  class Circuit {
  public:
    // Initialize input, output and first mult layer
    void Init(std::size_t n_clients, std::size_t batch_size);

    Circuit(std::size_t n_clients, std::size_t batch_size) : mBatchSize(batch_size), mClients(n_clients) {
      Init(n_clients, batch_size);
    }

    Circuit() : mBatchSize(1), mClients(1) {
      Init(mClients, mBatchSize);
    }

    // Append input gates
    std::shared_ptr<InputGate> Input(std::size_t owner_id);

    // Consolidates the batches for the inputs
    void CloseInputs() { for (auto input_layer : mInputLayers) input_layer.Close(); }

    // Append output gates
    std::shared_ptr<OutputGate> Output(std::size_t owner_id, std::shared_ptr<Gate> output);

    // Consolidates the batches for the outputs
    void CloseOutputs() { for (auto output_layer : mOutputLayers) output_layer.Close(); mIsClosed = true; }

    // Append addition gates
    std::shared_ptr<AddGate> Add(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right) {
      auto add_gate = std::make_shared<tp::AddGate>(left, right);
      mAddGates.emplace_back(add_gate);
      return add_gate;
    }

    // Append multiplication gates. These are added to the current
    // layer, which in turns add them to the current batch within that layer.
    std::shared_ptr<MultGate> Mult(std::shared_ptr<Gate> left, std::shared_ptr<Gate> right);

    // Used after the multiplications of a given layer have been
    // added, and new multiplications will be added to the next
    // layer. This closes the current layer, meaning that dummy gates
    // are appended if necessary to reach a full batch 
    void NewLayer();

    // Closes the current layer and does not open a new one. 
    void LastLayer();

    void SetNetwork(std::shared_ptr<scl::Network> network, std::size_t id) {
      if ( !mIsClosed )
	throw std::invalid_argument("Cannot set network without closing the circuit first");
      mNetwork = network;
      mID = id;
      mParties = network->Size();

      for (auto input_layer : mInputLayers) input_layer.SetNetwork(network, id);
      for (auto mult_layer : mMultLayers) mult_layer.SetNetwork(network, id);
      for (auto output_layer : mOutputLayers) output_layer.SetNetwork(network, id);

      mIsNetworkSet = true;
    }

    // Used to fetch input and output gates
    std::shared_ptr<InputGate> GetInputGate(std::size_t owner_id, std::size_t idx) { return mFlatInputGates[owner_id][idx]; }
    std::shared_ptr<InputGate> GetInputGate(std::size_t idx) { return mInputGates[idx]; }
    std::shared_ptr<OutputGate> GetOutputGate(std::size_t owner_id, std::size_t idx) { return mFlatOutputGates[owner_id][idx]; }
    std::shared_ptr<OutputGate> GetOutputGate(std::size_t idx) { return mOutputGates[idx]; }

    // Used to fetch the idx-th mult gate of the desired layer
    std::shared_ptr<MultGate> GetMultGate(std::size_t layer, std::size_t idx) {
      auto out = mFlatMultLayers[layer][idx];
      // auto out2 = mLayers[layer].GetMultBatch(idx/mBatchSize)->GetMultGate(idx % mBatchSize);
      // assert(mLayers[layer].GetMultBatch(idx/mBatchSize)->GetMultGate(idx % mBatchSize) == out); // PASSES
      return out;
    }

    // Set inputs for evaluations in the clear. Input is a vector of
    // vectors indicating the inputs of each party
    void SetClearInputs(std::vector<std::vector<FF>> inputs_per_client);

    // Set inputs for evaluations in the clear. Input is a vector of
    // inputs which get spread across all parties.  It is NOT checked
    // if there are enough inputs. This may segfault
    void SetClearInputsFlat(std::vector<FF> inputs);

    // Returns a vector of vectors with the outputs of each client
    std::vector<std::vector<FF>> GetClearOutputs();

    // Returns one long vector with all the outputs
    std::vector<FF> GetClearOutputsFlat();

    // DUMMY PREPROCESSING
    
    // Populates each batch with dummy preprocessing 
    void _DummyPrep(FF lambda) {
      for (auto input_gate : mInputGates) {
	input_gate->SetLambda(lambda);
      }
      for (std::size_t layer = 0; layer < GetDepth(); layer++) {
	for (auto mult_gate : mFlatMultLayers[layer]) {
	  mult_gate->SetDummyLambda(lambda);
	}
      }
      for (auto output_gate : mOutputGates) {
	output_gate->GetDummyLambda(); //populate outputs and add wires
      }

      // Now update packed sharings
      for (auto input_layer : mInputLayers) {
	for (auto input_batch : input_layer.mBatches) {
	  Vec lambda_A;
	  lambda_A.Reserve(mBatchSize);
	  for (std::size_t i = 0; i < mBatchSize; i++) {
	    lambda_A.Emplace(input_batch->GetInputGate(i)->GetDummyLambda());
	  }
	  scl::PRG prg;
	  auto poly = scl::details::EvPolyFromSecretsAndDegree(lambda_A, mBatchSize-1, prg);
	  Vec new_shares = scl::details::SharesFromEvPoly(poly, mParties);

	  input_batch->SetPreprocessing(new_shares[mID]);
	}
      }
      for (auto output_layer : mOutputLayers) {
	for (auto output_batch : output_layer.mBatches) {
	  Vec lambda_A;
	  lambda_A.Reserve(mBatchSize);
	  for (std::size_t i = 0; i < mBatchSize; i++) {
	    lambda_A.Emplace(output_batch->GetOutputGate(i)->GetDummyLambda());
	  }
	  scl::PRG prg;
	  auto poly = scl::details::EvPolyFromSecretsAndDegree(lambda_A, mBatchSize-1, prg);
	  Vec new_shares = scl::details::SharesFromEvPoly(poly, mParties);

	  output_batch->SetPreprocessing(new_shares[mID]);
	}
      }
      // Mults
      for (auto mult_layer : mMultLayers) {
	for (auto mult_batch : mult_layer.mBatches) {
	  Vec lambda_A;
	  Vec lambda_B;
	  Vec lambda_C;
	  lambda_A.Reserve(mBatchSize);
	  lambda_B.Reserve(mBatchSize);
	  lambda_C.Reserve(mBatchSize);
	  for (std::size_t i = 0; i < mBatchSize; i++) {
	    lambda_A.Emplace(mult_batch->GetMultGate(i)->GetLeft()->GetDummyLambda());
	    lambda_B.Emplace(mult_batch->GetMultGate(i)->GetRight()->GetDummyLambda());
	    lambda_C.Emplace(mult_batch->GetMultGate(i)->GetDummyLambda());
	  }
	  scl::PRG prg;
	  auto poly_A = scl::details::EvPolyFromSecretsAndDegree(lambda_A, mBatchSize-1, prg);
	  auto poly_B = scl::details::EvPolyFromSecretsAndDegree(lambda_B, mBatchSize-1, prg);
	  auto poly_C = scl::details::EvPolyFromSecretsAndDegree(lambda_C, mBatchSize-1, prg);
	  Vec new_shares_A = scl::details::SharesFromEvPoly(poly_A, mParties);
	  Vec new_shares_B = scl::details::SharesFromEvPoly(poly_B, mParties);
	  Vec new_shares_C = scl::details::SharesFromEvPoly(poly_C, mParties);

	  mult_batch->SetPreprocessing(new_shares_A[mID], new_shares_B[mID], \
				       new_shares_A[mID] * new_shares_B[mID] - new_shares_C[mID]);
	}
      }
      
      
    }

    // Populates each batch with dummy preprocessing (all zeros)
    void _DummyPrep();

    // Set all settable lambdas to a constant
    void SetDummyLambdas(FF lambda);

    // Populates the lambdas of addition and output gates
    void PopulateDummyLambdas();

    // Sets the lambdas for the output wires of addition and output
    // gates based on the lambdas for multiplications and input gates
    void PrepFromDummyLambdas();

    // FD PREPROCESSING

    void GenCorrelator();

    // void PopulateCorrelator() {
    //   mCorrelator.Populate();
    // }

    // The correlator is populated with dummy F.I. preprocessing
    void GenIndShrsDummy(FF lambda) {
      mCorrelator.GenIndShrsDummy(lambda);
    }
    void GenMultBatchDummy() {
      mCorrelator.GenMultBatchDummy();
    }
    void GenIOBatchDummy() {
      mCorrelator.GenIOBatchDummy();
    }

    void PopulateDummyCorrelator(FF lambda) {
      mCorrelator.GenPrepDummy(lambda);
    }

    // The correlator is populated with dummy F.I. preprocessing
    void PopulateDummyCorrelator() {
      mCorrelator.GenPrepDummy();
    }

    // Populates the mappings in the correlator so that the
    // F.I. preprocessing is mapped to different gates/batches
    void MapCorrToCircuit();

    // POPULATE THE CORRELATOR
    void GenIndShrsPartiesSend() { mCorrelator.GenIndShrsPartiesSend(); }
    void GenIndShrsPartiesReceive() { mCorrelator.GenIndShrsPartiesReceive(); }

    void GenUnpackedShrPartiesSend() { mCorrelator.GenUnpackedShrPartiesSend(); }
    void GenUnpackedShrPartiesReceive() { mCorrelator.GenUnpackedShrPartiesReceive(); }

    void GenZeroPartiesSend() { mCorrelator.GenZeroPartiesSend(); }
    void GenZeroPartiesReceive() { mCorrelator.GenZeroPartiesReceive(); }

    void GenZeroForProdPartiesSend() { mCorrelator.GenZeroForProdPartiesSend(); }
    void GenZeroForProdPartiesReceive() { mCorrelator.GenZeroForProdPartiesReceive(); }

    void GenProdPartiesSendP1() { mCorrelator.GenProdPartiesSendP1(); }
    void GenProdP1ReceivesAndSends() { mCorrelator.GenProdP1ReceivesAndSends(); }
    void GenProdPartiesReceive() { mCorrelator.GenProdPartiesReceive(); }

    // Prep inputs & outputs
    void PrepMultPartiesSendP1();
    void PrepMultP1ReceivesAndSends();
    void PrepMultPartiesReceive();

    void PrepIOPartiesSendOwner();

    void PrepIOOwnerReceives();

    void PrepOutputs();
    void PrepMults();

    // ONLINE PROTOCOL

    // Set inputs
    // Called separately by each party
    void SetInputs(std::vector<FF> inputs);

    // Input protocol
    void InputOwnerSendsP1();
    void InputP1Receives();
    void RunInput();

    // Multiplications in the i-th layer
    void MultP1Sends(std::size_t layer);
    void MultPartiesReceive(std::size_t layer);
    void MultPartiesSend(std::size_t layer);
    void MultP1Receives(std::size_t layer);

    void RunMult(std::size_t layer);

    // Output layers
    void OutputP1SendsMu();
    void OutputOwnerReceivesMu();
    void RunOutput();

    void RunProtocol();
    
    // Returns a vector with the outputs after computation
    std::vector<FF> GetOutputs();

    // Generates a synthetic circuit with the desired metrics
    static Circuit FromConfig(CircuitConfig config);

    // Fetch metrics
    std::size_t GetDepth() { return mMultLayers.size(); }
    std::size_t GetNInputs() { return mInputGates.size(); }
    std::size_t GetNOutputs() { return mOutputGates.size(); }
    std::size_t GetWidth() { return mWidth; }
    std::size_t GetSize() { return mSize; }
    std::size_t GetNInputBatches() {
      std::size_t sum(0);
      for (auto input_layer : mInputLayers) {
	sum += input_layer.GetSize();
      }
      return sum;
    }
    std::size_t GetNOutputBatches() {
      std::size_t sum(0);
      for (auto output_layer : mOutputLayers) {
	sum += output_layer.GetSize();
      }
      return sum;
    }
    std::size_t GetNMultBatches() {
      std::size_t sum(0);
      for (auto mult_layer : mMultLayers) {
	sum += mult_layer.GetSize();
      }
      return sum;
    }

    Correlator GetCorrelator() { return mCorrelator; }

    void SetThreshold(std::size_t threshold) {
      mCorrelator.SetThreshold(threshold);
      mCorrelator.PrecomputeVandermonde();
    }


  private:
    std::size_t mBatchSize;

    // List of layers. Each layer is itself a list of batches, which
    // is itself a list of batch_size gates
    std::vector<InputLayer> mInputLayers; // indexes represent parties
    std::vector<OutputLayer> mOutputLayers; // indexes represent parties
    std::vector<MultLayer> mMultLayers; // indexes represent layers
    
    // List of layers where each layer contains the gates
    // themselves, not the batches
    std::vector<std::vector<std::shared_ptr<MultGate>>> mFlatMultLayers; // Outer idx: layer
    std::vector<std::vector<std::shared_ptr<InputGate>>> mFlatInputGates; // Outer idx: client
    std::vector<std::vector<std::shared_ptr<OutputGate>>> mFlatOutputGates; // Outer idx: client

    // Lists with input and output gates
    std::vector<std::shared_ptr<InputGate>> mInputGates;
    std::vector<std::shared_ptr<OutputGate>> mOutputGates;
    std::vector<std::shared_ptr<AddGate>> mAddGates;

    // Correlator (for FIPrep)
    Correlator mCorrelator;

    // Metrics
    std::size_t mClients; // number of clients
    std::size_t mWidth=0;
    std::size_t mSize=0; // number of multiplications

    // Network-related
    std::shared_ptr<scl::Network> mNetwork;
    std::size_t mID;
    std::size_t mParties;

    // Flags
    bool mIsClosed;
    bool mIsNetworkSet;

    friend class Atlas;
  };

} // namespace tp

#endif  // CIRCUIT_H
