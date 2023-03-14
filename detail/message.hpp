#pragma once

#include <cstdint>
#include <string>

#include "noncopyable.hpp"

namespace asio_net {
namespace detail {

struct message : private noncopyable {
  message() = default;
  explicit message(std::string msg) : length(msg.length()), body(std::move(msg)) {}

  void clear() {
    length = 0;
    body.clear();
  }

  uint32_t length{};
  std::string body;
};

}  // namespace detail
}  // namespace asio_net
