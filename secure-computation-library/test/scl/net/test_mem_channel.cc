#include <catch2/catch.hpp>
#include <cstring>
#include <iostream>
#include <vector>

#include "scl/math.h"
#include "scl/net/mem_channel.h"
#include "scl/prg.h"

static inline bool Eq(const unsigned char* p, const unsigned char* q, int n) {
  while (n-- > 0 && *p++ == *q++)
    ;
  return n < 0;
}

static inline void PrintBuf(const unsigned char* b, std::size_t n) {
  for (std::size_t i = 0; i < n; ++i) {
    std::cout << (int)b[i] << " ";
  }
  std::cout << ""
            << "\n";
}

TEST_CASE("InMemoryChannel", "[network]") {
  auto channels = scl::InMemoryChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  scl::PRG prg;
  unsigned char data_in[200] = {0};
  prg.Next(data_in, 200);

  SECTION("Send and receive") {
    unsigned char data_out[200] = {0};
    chl0->Send(data_in, 200);
    chl1->Recv(data_out, 200);
    REQUIRE(Eq(data_in, data_out, 200));
  }

  chl0->Flush();
  chl1->Flush();

  SECTION("Send chunked") {
    unsigned char data_out[200] = {0};

    chl0->Send(data_in, 50);
    chl0->Send(data_in + 50, 50);
    chl0->Send(data_in + 100, 100);
    chl1->Recv(data_out, 200);

    REQUIRE(Eq(data_in, data_out, 200));
  }

  chl0->Flush();
  chl1->Flush();

  SECTION("Recv chunked") {
    unsigned char data_out[200] = {0};
    chl0->Send(data_in, 100);
    chl0->Send(data_in + 100, 100);
    chl1->Recv(data_out, 100);
    chl1->Recv(data_out + 100, 100);

    REQUIRE(Eq(data_in, data_out, 200));
  }

  chl0->Flush();
  chl1->Flush();

  SECTION("Send trivial data") {
    scl::Channel* c0 = chl0.get();
    scl::Channel* c1 = chl1.get();
    int x = 123;
    c0->Send(x);
    int y;
    c1->Recv(y);
    REQUIRE(x == y);
  }

  chl0->Flush();
  chl1->Flush();

  SECTION("Send vector trivial data") {
    scl::Channel* c0 = chl0.get();
    scl::Channel* c1 = chl1.get();
    std::vector<long> data = {1, 2, 3, 4, 11111111};
    c0->Send(data);
    std::vector<long> recv(data.size());
    c1->Recv(recv);
    REQUIRE(data == recv);
  }

  using FF = scl::FF<61>;
  using Vec = scl::Vec<FF>;

  chl0->Flush();
  chl1->Flush();

  SECTION("Send Vec") {
    scl::Channel* c0 = chl0.get();
    scl::Channel* c1 = chl1.get();
    Vec v = {FF(1), FF(5), FF(2) - FF(10)};
    c0->Send(v);
    Vec w;
    c1->Recv(w);
    REQUIRE(v.Equals(w));
  }

  chl0->Flush();
  chl1->Flush();

  using Mat = scl::Mat<FF>;

  SECTION("Send mat") {
    scl::Channel* c0 = chl0.get();
    scl::Channel* c1 = chl1.get();
    auto m = Mat::Random(5, 7, prg);
    c0->Send(m);
    Mat a;
    c1->Recv(a);
    REQUIRE(m.Equals(a));
  }

  SECTION("Send self") {
    auto c = scl::InMemoryChannel::CreateSelfConnecting();

    c->Send(data_in, 20);
    c->Send(data_in + 20, 100);
    c->Send(data_in + 120, 80);

    unsigned char data_out[200] = {0};
    c->Recv(data_out, 10);
    c->Recv(data_out + 10, 100);
    c->Recv(data_out + 110, 90);

    REQUIRE(Eq(data_in, data_out, 200));
  }
}
