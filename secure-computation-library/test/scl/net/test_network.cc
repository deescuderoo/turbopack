#include <catch2/catch.hpp>
#include <thread>

#include "scl/net/config.h"
#include "scl/net/network.h"

TEST_CASE("Network", "[network]") {
  SECTION("Mock") {
    auto mock = scl::Network::CreateMock(0, 3);

    auto network = std::get<0>(mock);
    auto remotes = std::get<1>(mock);

    REQUIRE(network.Size() == 3);
    REQUIRE(remotes[0] == nullptr);

    remotes[1]->Send((int)123);

    int x;
    network.Party(1)->Recv(x);

    REQUIRE(x == 123);

    network.Party(0)->Send((int)1111);
    network.Party(0)->Recv(x);

    REQUIRE(x == 1111);
  }

  SECTION("Full") {
    auto networks = scl::Network::CreateFullInMemory(3);
    REQUIRE(networks.size() == 3);

    auto network0 = networks[0];
    auto network1 = networks[1];

    network0.Party(1)->Send((int)444);

    int x;
    network1.Party(0)->Recv(x);

    REQUIRE(x == 444);

    auto network2 = networks[2];
    network2.Party(0)->Send((int)555);

    network0.Party(2)->Recv(x);

    REQUIRE(x == 555);
  }

  SECTION("TCP") {
    scl::Network network0, network1, network2;

    std::thread t0([&]() {
      network0 = scl::Network::Create(scl::NetworkConfig::Localhost(0, 3));
    });
    std::thread t1([&]() {
      network1 = scl::Network::Create(scl::NetworkConfig::Localhost(1, 3));
    });
    std::thread t2([&]() {
      network2 = scl::Network::Create(scl::NetworkConfig::Localhost(2, 3));
    });

    t0.join();
    t1.join();
    t2.join();

    for (std::size_t i = 0; i < 3; ++i) {
      // Alive doesn't exist on InMemoryChannel
      if (i != 0) {
        REQUIRE(((scl::TcpChannel*)network0.Party(i))->Alive());
      }
      if (i != 1) {
        REQUIRE(((scl::TcpChannel*)network1.Party(i))->Alive());
      }
      if (i != 2) {
        REQUIRE(((scl::TcpChannel*)network2.Party(i))->Alive());
      }
    }

    network0.Party(2)->Send((int)5555);

    int x;
    network2.Party(0)->Recv(x);

    REQUIRE(x == 5555);
  }
}
