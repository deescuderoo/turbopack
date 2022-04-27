#include <iostream>
#include <chrono>
#include <thread>

#include "tp/atlas.h"
#include "misc.h"

#define DELIM std::cout << "========================================\n"

#define DEBUG true
#define THREAD false

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


  // ATLAS
  tp::Atlas atlas(n_parties, t);
  atlas.SetCircuit(circuit);

  // Prep
  DELIM;
  std::cout << "ATLAS: Running preprocessing\n";

  START_TIMER(atlas_prep);
  // This has to be done BEFORE setting the inputs below
  if (THREAD) {
    std::thread t_PrepPartiesSend( &tp::Atlas::PrepPartiesSend, &atlas );
    atlas.PrepPartiesReceive();
    t_PrepPartiesSend.join();
  } else {
    atlas.PrepPartiesSend(); 
    atlas.PrepPartiesReceive();
  }


  STOP_TIMER(atlas_prep);


  std::vector<tp::FF> result;
  if (id == 0){
    std::vector<tp::FF> inputs{tp::FF(0432432), tp::FF(54982)};
    atlas.GetCircuit().SetClearInputsFlat(inputs);
    result = atlas.GetCircuit().GetClearOutputsFlat();
    atlas.GetCircuit().SetInputs(inputs);
  }


  DELIM;
  std::cout << "ATLAS: Running online\n";
  START_TIMER(atlas_online);
  // Input protocol (inputs are already set from above)

  atlas.InputPartiesSendOwners(); 
  atlas.InputOwnersReceiveAndSendParties(); 
  atlas.InputPartiesReceive(); 

  // Multiplications 
    
       for (std::size_t layer = 0; layer < circuit_config.depth; layer++) {
	 if (THREAD) {
	   std::thread t_MultPartiesSendP1( &tp::Atlas::MultPartiesSendP1, &atlas, layer); 
	   std::thread t_MultP1ReceivesAndSendsParties( &tp::Atlas::MultP1ReceivesAndSendsParties, &atlas, layer); 
	   atlas.MultPartiesReceive(layer);
	   t_MultPartiesSendP1.join();
	   t_MultP1ReceivesAndSendsParties.join();
	 } else {
	   atlas.MultPartiesSendP1(layer); 
	   atlas.MultP1ReceivesAndSendsParties(layer); 
	   atlas.MultPartiesReceive(layer);
	 }
       }
  // Output protocol
  atlas.OutputPartiesSendOwners(); 
  atlas.OutputOwnersReceive(); 

  STOP_TIMER(atlas_online);

  if (id == 0) {
    // std::cout << "\nOUTPUT = " << atlas.GetOutput(0,0) << "\nREAL = " << result[0] << "\n";
    assert( atlas.GetOutput(0,0) == result[0] );
  }

  std::cout << "\nclosing the network ...\n";
  // technically not necessary as channels are destroyed when their dtor is
  // called.
  network.Close();

}
