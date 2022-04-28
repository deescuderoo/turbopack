#include <iostream>
#include <chrono>
#include <thread>

#include "tp/dn07.h"
#include "misc.h"

#define DELIM std::cout << "========================================\n"

#define DEBUG true
#define THREAD true

#define PRINT(x) if (DEBUG) std::cout << x << "\n";

inline std::size_t ValidateN(const std::size_t n) {
  assert(n > 3
	 && (((n-1) % 4) == 0)
	 );
  return n;			
}

inline std::size_t ValidateId(const std::size_t id, const std::size_t n) {
  if ( id >= n )
    throw std::invalid_argument("ID cannot be larger than number of parties");
  return id;
}

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cout << "usage: " << argv[0] << " [N] [id] [size] [depth]\n";
    return 0;
  }

  std::size_t n = ValidateN(std::stoul(argv[1]));
  std::size_t t = (n - 1) / 2;
  std::size_t id = ValidateId(std::stoul(argv[2]), n);
  std::size_t size = std::stoul(argv[3]);
  std::size_t depth = std::stoul(argv[4]);
  std::size_t width = size/depth;

  DELIM;
  std::cout << "Running benchmark with N " << n << ", size " <<
    size << ", width " << width << " and depth " << depth << "\n";
  DELIM;

  auto config = scl::NetworkConfig::Localhost(id, n);
  // std::cout << "Config:"
  //           << "\n";
  // for (const auto& party : config.Parties()) {
  //   std::cout << " -- " << party.id << ", " << party.hostname << ", "
  //             << party.port << "\n";
  // }

  std::cout << "Connecting ..."
            << "\n";
  auto network = scl::Network::Create(config);

  std::cout << "Done!\n";

  std::size_t batch_size = (t + 2)/2;
  std::size_t n_parties = t + 2*(batch_size - 1) + 1;
  // std::size_t n_clients = n_parties;

  tp::CircuitConfig circuit_config;
  circuit_config.n_parties = n_parties;
  circuit_config.inp_gates = std::vector<std::size_t>(n_parties, 0);
  circuit_config.inp_gates[0] = 2;
  circuit_config.out_gates = std::vector<std::size_t>(n_parties, 0);
  circuit_config.out_gates[0] = 2;
  circuit_config.width = width;
  circuit_config.depth = depth;
  circuit_config.batch_size = batch_size;
    
  auto circuit = tp::Circuit::FromConfig(circuit_config);

  circuit.SetNetwork(std::make_shared<scl::Network>(network), id);

  circuit.GenCorrelator();
  circuit.SetThreshold(t);

  //////////////////////////////////////////


  // DN07
  tp::DN07 dn07(n_parties, t);
  dn07.SetCircuit(circuit);

  // Prep
  DELIM;
  std::cout << "DN07: Running preprocessing\n";

  START_TIMER(dn07_prep);
  // This has to be done BEFORE setting the inputs below
  if (THREAD) {
    std::thread t_PrepPartiesSend( &tp::DN07::PrepPartiesSend, &dn07 );
    dn07.PrepPartiesReceive();
    t_PrepPartiesSend.join();
  } else {
    dn07.PrepPartiesSend(); 
    dn07.PrepPartiesReceive();
  }


  STOP_TIMER(dn07_prep);


  std::vector<tp::FF> result;
  if (id == 0){
    std::vector<tp::FF> inputs{tp::FF(0432432), tp::FF(54982)};
    dn07.GetCircuit().SetClearInputsFlat(inputs);
    result = dn07.GetCircuit().GetClearOutputsFlat();
    dn07.GetCircuit().SetInputs(inputs);
  }


  DELIM;
  std::cout << "DN07: Running online\n";
  START_TIMER(dn07_online);
  // Input protocol (inputs are already set from above)

  dn07.InputPartiesSendOwners(); 
  dn07.InputOwnersReceiveAndSendParties(); 
  dn07.InputPartiesReceive(); 

  // Multiplications 
    
       for (std::size_t layer = 0; layer < circuit_config.depth; layer++) {
	 if (THREAD) {
	   std::thread t_MultPartiesSendP1( &tp::DN07::MultPartiesSendP1, &dn07, layer); 
	   dn07.MultP1Receives(layer);
	   t_MultPartiesSendP1.join();

	   std::thread t_MultP1SendsParties( &tp::DN07::MultP1SendsParties, &dn07, layer); 
	   dn07.MultPartiesReceive(layer);
	   t_MultP1SendsParties.join();
	 } else {
	   dn07.MultPartiesSendP1(layer); 
	   dn07.MultP1ReceivesAndSendsParties(layer); 
	   dn07.MultPartiesReceive(layer);
	 }
       }
  // Output protocol
  dn07.OutputPartiesSendOwners(); 
  dn07.OutputOwnersReceive(); 

  STOP_TIMER(dn07_online);

  if (id == 0) {
    // std::cout << "\nOUTPUT = " << dn07.GetOutput(0,0) << "\nREAL = " << result[0] << "\n";
    assert( dn07.GetOutput(0,0) == result[0] );
  }

  std::cout << "\nclosing the network ...\n";
  // technically not necessary as channels are destroyed when their dtor is
  // called.
  network.Close();

}
