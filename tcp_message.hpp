#pragma once

#include <cstdint>
#include <string>

#include "detail/noncopyable.hpp"

namespace asio_net {

struct tcp_message : private noncopyable {
  tcp_message() = default;
  explicit tcp_message(std::string msg) : length(msg.length()), body(std::move(msg)) {}

  void clear() {
    length = 0;
    body.clear();
  }

  uint32_t length{};
  std::string body;
};

}  // namespace asio_net
