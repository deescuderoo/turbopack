#include <catch2/catch.hpp>
#include <iostream>
#include <memory>
#include <thread>

#include "scl/net/tcp_utils.h"
#include "scl/net/threaded_sender.h"
#include "scl/prg.h"
#include "util.h"

TEST_CASE("ThreadedSender", "[network]") {
  SECTION("Connect and send") {
    auto port = scl_tests::GetPort();

    std::shared_ptr<scl::Channel> client, server;

    std::thread clt([&]() {
      int socket = scl::details::ConnectAsClient("0.0.0.0", port);
      client = std::make_shared<scl::ThreadedSenderChannel>(socket);
    });

    std::thread srv([&]() {
      int ssock = scl::details::CreateServerSocket(port, 1);
      auto ac = scl::details::AcceptConnection(ssock);
      server = std::make_shared<scl::ThreadedSenderChannel>(ac.socket);
      scl::details::CloseSocket(ssock);
    });

    clt.join();
    srv.join();

    scl::PRG prg;
    unsigned char send[200] = {0};
    unsigned char recv[200] = {0};
    prg.Next(send, 200);

    client->Send(send, 100);
    client->Send(send + 100, 100);

    server->Recv(recv, 20);
    server->Recv(recv + 20, 180);

    client->Close();
    server->Close();

    REQUIRE(scl_tests::BufferEquals(send, recv, 200));
  }
}
