#pragma once

#include <cstdint>
#include <string>

namespace asio_tcp {

struct tcp_message {
  tcp_message() = default;
  tcp_message(std::string msg) : header(msg.length()), body(std::move(msg)) {}

  uint32_t header;  // body length
  std::string body;
};

}  // namespace asio_tcp
