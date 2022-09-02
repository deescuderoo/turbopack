#include "scl/net/threaded_sender.h"

#include <future>

scl::ThreadedSenderChannel::ThreadedSenderChannel(int socket)
    : mChannel(scl::TcpChannel(socket)) {
  mSender = std::async(std::launch::async, [&]() {
    while (true) {
      auto data = mSendBuffer.Peek();
      if (!mChannel.Alive()) {
        break;
      }
      mChannel.Send(data.data(), data.size());
      mSendBuffer.PopFront();
    }
  });
}

void scl::ThreadedSenderChannel::Close() {
  mChannel.Close();
  unsigned char stop_signal = 1;
  Send(&stop_signal, 1);
  mSender.wait();
}
