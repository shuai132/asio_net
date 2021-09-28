#pragma once

#include "asio.hpp"

namespace asio_udp {

using asio::ip::udp;

class udp_client {
 public:
  explicit udp_client(asio::io_context& io_context)
      : socket_(io_context, udp::endpoint(udp::v4(), 0)) {}

  void do_send(void* data, size_t size, const udp::endpoint& endpoint) {
    auto keeper = std::make_unique<std::string>((char*)data, size);
    socket_.async_send_to(
        asio::buffer(keeper->data(), keeper->length()), endpoint,
        [keeper = std::move(keeper)](std::error_code /*ec*/,
                                     std::size_t /*bytes_sent*/) {});
  }

 private:
  udp::socket socket_;
};

}  // namespace asio_udp
