#pragma once

#include <utility>

#include "asio.hpp"
#include "tcp_channel.hpp"
#include "type.h"

namespace asio_net {

class tcp_session : public tcp_channel, public std::enable_shared_from_this<tcp_session> {
 public:
  explicit tcp_session(tcp::socket socket, const PackOption& pack_option, const uint32_t& max_body_size)
      : tcp_channel(socket_, pack_option, max_body_size), socket_(std::move(socket)) {}

  void start() {
    do_read_start(shared_from_this());
  }

 private:
  tcp::socket socket_;
};

class tcp_server {
 public:
  tcp_server(asio::io_context& io_context, uint16_t port, PackOption pack_option = PackOption::DISABLE, uint32_t max_body_size = UINT32_MAX)
      : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), pack_option_(pack_option), max_body_size_([&] {
          if (pack_option == PackOption::DISABLE) {
            max_body_size = std::min<uint32_t>(1024, max_body_size);
            max_body_size = std::max<uint32_t>(32, max_body_size);
          }
          return max_body_size;
        }()) {}

 public:
  void start(bool loop = false) {
    do_accept();
    if (loop) {
      io_context_.run();
    }
  }

 public:
  std::function<void(std::weak_ptr<tcp_session>)> on_session;

 private:
  void do_accept() {
    acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
      if (!ec) {
        auto session = std::make_shared<tcp_session>(std::move(socket), pack_option_, max_body_size_);
        session->start();
        if (on_session) on_session(session);
      }

      do_accept();
    });
  }

 private:
  asio::io_context& io_context_;
  tcp::acceptor acceptor_;
  PackOption pack_option_;
  uint32_t max_body_size_;
};

}  // namespace asio_net
