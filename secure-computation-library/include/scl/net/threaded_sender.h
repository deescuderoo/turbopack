/**
 * @file threaded_sender.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#ifndef _SCL_NET_THREADED_SENDER_H
#define _SCL_NET_THREADED_SENDER_H

#include <cstddef>
#include <future>

#include "scl/net/channel.h"
#include "scl/net/shared_deque.h"
#include "scl/net/tcp_channel.h"

namespace scl {

/**
 * @brief A decorator for scl::TcpChannel that runs Send calls in a separate
 * thread.
 *
 * The purpose of this class is to avoid situations where calls to Send may
 * block, for example if we're trying to send more that what can fit in the TCP
 * window.
 */
class ThreadedSenderChannel final : public Channel {
 public:
  /**
   * @brief Create a new threaded sender channel.
   * @param socket an open socket used to construct scl::TcpChannel
   */
  ThreadedSenderChannel(int socket);

  void Close() override;

  void Send(const unsigned char* src, std::size_t n) override {
    mSendBuffer.PushBack({src, src + n});
  };

  void Recv(unsigned char* dst, std::size_t n) override {
    mChannel.Recv(dst, n);
  };

 private:
  TcpChannel mChannel;
  details::SharedDeque<std::vector<unsigned char>> mSendBuffer;
  std::future<void> mSender;
};

}  // namespace scl

#endif /* _SCL_NET_THREADED_SENDER_H */
