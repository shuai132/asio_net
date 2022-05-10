#pragma once

#include "noncopyable.hpp"
#include "tcp_channel.hpp"

namespace asio_net {

class tcp_client : public tcp_channel {
 public:
  explicit tcp_client(asio::io_context& io_context, uint32_t max_body_size = 4096)
      : tcp_channel(socket_, max_body_size_), socket_(io_context), max_body_size_(max_body_size) {}

  void open(const std::string& ip, const std::string& port) {
    tcp::resolver resolver(socket_.get_executor());
    do_connect(resolver.resolve(ip, port));
  }

 public:
  std::function<void()> on_open;

 private:
  void do_connect(const tcp::resolver::results_type& endpoints) {
    asio::async_connect(socket_, endpoints, [this](std::error_code ec, const tcp::endpoint&) {
      if (!ec) {
        if (on_open) on_open();
        do_read_start();
      }
    });
  }

 private:
  tcp::socket socket_;
  uint32_t max_body_size_;
};

}  // namespace asio_net
