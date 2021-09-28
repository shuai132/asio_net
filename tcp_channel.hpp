#pragma once

#include <utility>

#include "asio.hpp"
#include "tcp_message.hpp"

namespace asio_tcp {

using asio::ip::tcp;

class tcp_channel {
 public:
  tcp_channel(tcp::socket& socket, const uint32_t& max_body_size)
      : socket_(socket), max_body_size_(max_body_size) {}

  void send(std::string msg) { do_write(std::move(msg)); }

  void close() { do_close(); }

  bool is_open() const { return socket_.is_open(); }

  tcp::endpoint local_endpoint() { return socket_.local_endpoint(); }

  tcp::endpoint remote_endpoint() { return socket_.remote_endpoint(); }

 public:
  std::function<void()> onOpen;
  std::function<void()> onClose;
  std::function<void(std::string)> onData;

 protected:
  void do_read_start(std::shared_ptr<tcp_channel> self = nullptr) {
    do_read_header(std::move(self));
  }

 private:
  void do_read_header(std::shared_ptr<tcp_channel> self = nullptr) {
    asio::async_read(socket_,
                     asio::buffer(&read_msg_.header, sizeof(read_msg_.header)),
                     [this, self = std::move(self)](
                         std::error_code ec, std::size_t /*length*/) mutable {
                       if (!ec && read_msg_.header <= max_body_size_) {
                         do_read_body(std::move(self));
                       } else {
                         do_close();
                       }
                     });
  }

  void do_read_body(std::shared_ptr<tcp_channel> self = nullptr) {
    read_msg_.body.resize(read_msg_.header);
    asio::async_read(socket_, asio::buffer(read_msg_.body),
                     [this, self = std::move(self)](
                         std::error_code ec, std::size_t /*length*/) mutable {
                       if (!ec) {
                         auto msg = std::move(read_msg_.body);
                         read_msg_ = {};
                         if (onData) onData(std::move(msg));
                         do_read_header(std::move(self));
                       } else {
                         socket_.close();
                       }
                     });
  }

  void do_write(std::string msg) {
    auto keeper = std::make_unique<tcp_message>(std::move(msg));
    asio::async_write(socket_,
                      asio::buffer(&keeper->header, sizeof(keeper->header)),
                      [this](std::error_code ec, std::size_t /*length*/) {
                        if (ec) {
                          do_close();
                        }
                      });
    asio::async_write(socket_, asio::buffer(keeper->body),
                      [this, keeper = std::move(keeper)](
                          std::error_code ec, std::size_t /*length*/) {
                        if (ec) {
                          do_close();
                        }
                      });
  }

  void do_close() {
    if (!is_open()) return;
    socket_.close();
    if (onClose) onClose();
  }

 private:
  tcp::socket& socket_;
  const uint32_t& max_body_size_;
  tcp_message read_msg_;
};

}  // namespace asio_tcp
