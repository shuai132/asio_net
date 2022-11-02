#pragma once

#include "asio.hpp"
#include "detail/noncopyable.hpp"

namespace asio_net {

using asio::ip::udp;

class udp_client : private noncopyable {
 public:
  explicit udp_client(asio::io_context& io_context) : socket_(io_context, udp::endpoint(udp::v4(), 0)) {}

  void send_to(std::string data, const udp::endpoint& endpoint) {
    auto keeper = std::make_unique<std::string>(std::move(data));
    auto k_data = keeper->data();
    auto k_size = keeper->size();
    socket_.async_send_to(asio::buffer(k_data, k_size), endpoint,
                          [keeper = std::move(keeper)](const std::error_code& /*ec*/, std::size_t /*bytes_sent*/) {});
  }

  void send_to(void* data, size_t size, const udp::endpoint& endpoint) {
    send_to(std::string((char*)data, size), endpoint);
  }

 private:
  udp::socket socket_;
};

}  // namespace asio_net
