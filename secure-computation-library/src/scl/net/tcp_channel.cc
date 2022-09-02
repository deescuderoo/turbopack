#include "scl/net/tcp_channel.h"

#include "scl/net/tcp_utils.h"

void scl::TcpChannel::Close() {
  if (!mAlive) return;

  const auto err = scl::details::CloseSocket(mSocket);
  if (err < 0) scl::details::ThrowError("error closing socket");
  mAlive = false;
}

void scl::TcpChannel::Send(const unsigned char* src, std::size_t n) {
  std::size_t rem = n;
  std::size_t offset = 0;

  while (rem > 0) {
    auto sent = scl::details::WriteToSocket(mSocket, src + offset, rem);
    if (sent < 0) scl::details::ThrowError("write failed");
    rem -= sent;
    offset += sent;
  }
}

void scl::TcpChannel::Recv(unsigned char* dst, std::size_t n) {
  std::size_t rem = n;
  std::size_t offset = 0;

  while (rem > 0) {
    auto recv = scl::details::ReadFromSocket(mSocket, dst + offset, rem);
    if (!recv) break;
    if (recv < 0) scl::details::ThrowError("read failed");

    rem -= recv;
    offset += recv;
  }
}
