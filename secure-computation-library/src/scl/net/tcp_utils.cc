#include "scl/net/tcp_utils.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <thread>

int scl::details::CreateServerSocket(int port, int backlog) {
  int err;
  int ssock = ::socket(AF_INET, SOCK_STREAM, 0);
  if (ssock < 0) scl::details::ThrowError("could not acquire server socket");

  int opt = 1;
  auto options = SO_REUSEADDR | SO_REUSEPORT;
  err = ::setsockopt(ssock, SOL_SOCKET, options, &opt, sizeof(opt));
  if (err < 0) scl::details::ThrowError("could not set socket options");

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ::htons(INADDR_ANY);
  addr.sin_port = ::htons(port);

  struct sockaddr* addr_ptr = (struct sockaddr*)&addr;

  err = ::bind(ssock, addr_ptr, sizeof(addr));
  if (err < 0) scl::details::ThrowError("could not bind socket");

  err = ::listen(ssock, backlog);
  if (err < 0) scl::details::ThrowError("could not listen on socket");

  return ssock;
}

scl::details::AcceptedConnection scl::details::AcceptConnection(
    int server_socket) {
  scl::details::AcceptedConnection ac;
  ac.socket_info = std::make_shared<struct sockaddr>();
  struct sockaddr_in addr;
  auto addrsize = sizeof(addr);
  ac.socket =
      ::accept(server_socket, ac.socket_info.get(), (socklen_t*)&addrsize);
  if (ac.socket < 0) scl::details::ThrowError("could not accept connection");
  return ac;
}

std::string scl::details::GetAddress(
    scl::details::AcceptedConnection connection) {
  struct sockaddr_in* s =
      reinterpret_cast<struct sockaddr_in*>(connection.socket_info.get());
  return inet_ntoa(s->sin_addr);
}

int scl::details::ConnectAsClient(std::string hostname, int port) {
  using namespace std::chrono_literals;

  int sock = ::socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) scl::details::ThrowError("could not acquire socket");

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = ::htons(port);

  int err = ::inet_pton(AF_INET, hostname.c_str(), &(addr.sin_addr));
  if (err == 0) throw std::runtime_error("invalid hostname");
  if (err < 0) scl::details::ThrowError("invalid address family");

  while (::connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    std::this_thread::sleep_for(300ms);

  return sock;
}

int scl::details::CloseSocket(int socket) { return ::close(socket); }

int scl::details::ReadFromSocket(int socket, unsigned char* dst,
                                 std::size_t n) {
  return ::read(socket, dst, n);
}

int scl::details::WriteToSocket(int socket, const unsigned char* src,
                                std::size_t n) {
  return ::write(socket, src, n);
}
