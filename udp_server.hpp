#pragma once

#include "asio.hpp"
#include "detail/noncopyable.hpp"

namespace asio_net {

using asio::ip::udp;

class udp_server : private noncopyable {
 public:
  udp_server(asio::io_context& io_context, short port, uint16_t max_length = 4096)
      : socket_(io_context, udp::endpoint(udp::v4(), port)), max_length_(max_length) {
    data_.resize(max_length);
    do_receive();
  }

  std::function<void(uint8_t* data, size_t size, udp::endpoint from)> on_data;

 private:
  void do_receive() {
    socket_.async_receive_from(asio::buffer((void*)data_.data(), max_length_), from_endpoint_, [this](std::error_code ec, std::size_t bytes_recv) {
      if (!ec && bytes_recv > 0) {
        if (on_data) on_data((uint8_t*)data_.data(), bytes_recv, from_endpoint_);
        do_receive();
      }
    });
  }

 private:
  udp::socket socket_;
  udp::endpoint from_endpoint_;
  uint16_t max_length_;
  std::string data_;
};

}  // namespace asio_net
