#include "scl/net/discovery/client.h"

#include <memory>

#include "scl/net/tcp_utils.h"

using Client = scl::DiscoveryClient;

scl::NetworkConfig Client::Run(unsigned id, int port) {
  auto socket = scl::details::ConnectAsClient(mHostname, mPort);
  std::shared_ptr<scl::Channel> server =
      std::make_shared<scl::TcpChannel>(socket);

  Client::SendIdAndPort discovery(id, port);
  return scl::Evaluate(discovery, server);
}

Client::ReceiveNetworkConfig Client::SendIdAndPort::Run(
    std::shared_ptr<scl::Channel> ctx) {
  ctx->Send(mId);
  ctx->Send(mPort);
  return Client::ReceiveNetworkConfig{mId};
}

static inline std::string ReceiveHostname(std::shared_ptr<scl::Channel> ctx) {
  std::size_t len;
  ctx->Recv(len);
  auto buf = std::make_unique<char[]>(len);
  ctx->Recv(reinterpret_cast<unsigned char*>(buf.get()), len);
  return std::string(buf.get(), buf.get() + len);
}

scl::NetworkConfig Client::ReceiveNetworkConfig::Finalize(
    std::shared_ptr<scl::Channel> ctx) {
  std::size_t number_of_parties;
  ctx->Recv(number_of_parties);

  std::vector<scl::Party> parties(number_of_parties);
  for (std::size_t i = 0; i < number_of_parties; ++i) {
    scl::Party party;
    ctx->Recv(party.id);
    ctx->Recv(party.port);
    auto hostname = ReceiveHostname(ctx);
    party.hostname = hostname;
    parties[party.id] = party;
  }

  return scl::NetworkConfig{mId, parties};
}
