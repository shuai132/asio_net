#pragma once

#include <utility>

#include "asio.hpp"
#include "detail/log.h"
#include "detail/noncopyable.hpp"
#include "tcp_message.hpp"

namespace asio_net {

using asio::ip::tcp;

class tcp_channel : private noncopyable {
 public:
  tcp_channel(tcp::socket& socket, const uint32_t& max_body_size) : socket_(socket), max_body_size_(max_body_size) {
    asio_net_LOGD("tcp_channel: %p", this);
  }

  ~tcp_channel() {
    asio_net_LOGD("~tcp_channel: %p", this);
  }

  void send(std::string msg) {
    do_write(std::move(msg));
  }

  void close() {
    asio::post(socket_.get_executor(), [this] {
      do_close();
    });
  }

  bool is_open() const {
    return socket_.is_open();
  }

  tcp::endpoint local_endpoint() {
    return socket_.local_endpoint();
  }

  tcp::endpoint remote_endpoint() {
    return socket_.remote_endpoint();
  }

 public:
  std::function<void()> on_close;
  std::function<void(std::string)> on_data;

 protected:
  void do_read_start(std::shared_ptr<tcp_channel> self = nullptr) {
    do_read_header(std::move(self));
  }

 private:
  void do_read_header(std::shared_ptr<tcp_channel> self = nullptr) {
    asio::async_read(socket_, asio::buffer(&read_msg_.length, sizeof(read_msg_.length)),
                     [this, self = std::move(self)](std::error_code ec, std::size_t /*length*/) mutable {
                       if (ec) {
                         do_close();
                       }
                       if (read_msg_.length <= max_body_size_) {
                         do_read_body(std::move(self));
                       } else {
                         asio_net_LOGE("read: body size=%u > max_body_size=%u", read_msg_.length, max_body_size_);
                         do_close();
                       }
                     });
  }

  void do_read_body(std::shared_ptr<tcp_channel> self = nullptr) {
    read_msg_.body.resize(read_msg_.length);
    asio::async_read(socket_, asio::buffer(read_msg_.body), [this, self = std::move(self)](std::error_code ec, std::size_t /*length*/) mutable {
      if (!ec) {
        auto msg = std::move(read_msg_.body);
        read_msg_.clear();
        if (on_data) on_data(std::move(msg));
        do_read_header(std::move(self));
      } else {
        socket_.close();
      }
    });
  }

  void do_write(std::string msg) {
    auto keeper = std::make_unique<tcp_message>(std::move(msg));
    if (keeper->length > max_body_size_) {
      asio_net_LOGE("write: body size=%u > max_body_size=%u", keeper->length, max_body_size_);
      do_close();
    }

    auto buffer = {
        asio::buffer(&keeper->length, sizeof(keeper->length)),
        asio::buffer(keeper->body),
    };
    asio::async_write(socket_, buffer, [this, keeper = std::move(keeper)](std::error_code ec, std::size_t /*length*/) {
      if (ec) {
        do_close();
      }
    });
  }

  void do_close() {
    if (!is_open()) return;
    try {
      socket_.close();
    } catch (std::exception& e) {
      asio_net_LOGW("do_close: %s", e.what());
    }
    if (on_close) on_close();
  }

 private:
  tcp::socket& socket_;
  const uint32_t& max_body_size_;
  tcp_message read_msg_;
};

}  // namespace asio_net
