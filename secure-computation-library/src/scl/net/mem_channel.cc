#include "scl/net/mem_channel.h"

#include <cstring>

void scl::InMemoryChannel::Send(const unsigned char* src, std::size_t n) {
  mOut->PushBack(std::vector<unsigned char>(src, src + n));
}

void scl::InMemoryChannel::Recv(unsigned char* dst, std::size_t n) {
  std::size_t rem = n;

  // if there's any leftovers from previous calls to recv, then we retrieve
  // those first.
  const auto leftovers = mOverflow.size();
  if (leftovers > 0) {
    const auto to_copy = leftovers > rem ? rem : leftovers;
    auto data = mOverflow.data();
    std::memcpy(dst, data, to_copy);
    rem -= to_copy;
    mOverflow = std::vector<unsigned char>(mOverflow.begin() + to_copy,
                                           mOverflow.end());
  }

  while (rem > 0) {
    auto data = mIn->Pop();
    const auto to_copy = data.size() > rem ? rem : data.size();
    std::memcpy(dst + (n - rem), data.data(), to_copy);
    rem -= to_copy;

    // if we didn't copy all of data, then rem == 0 and we need to save what
    // remains to overflow
    if (to_copy < data.size()) {
      const auto leftovers = data.size() - to_copy;
      const auto old_size = mOverflow.size();
      mOverflow.reserve(old_size + leftovers);
      mOverflow.insert(mOverflow.begin() + old_size, data.begin() + to_copy,
                       data.end());
    }
  }
}
