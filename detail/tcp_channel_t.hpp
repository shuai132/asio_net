#pragma once

#include <utility>

#include "asio.hpp"
#include "log.h"
#include "message.hpp"
#include "noncopyable.hpp"
#include "type.h"

namespace asio_net {
namespace detail {

template <typename T>
class tcp_channel_t : private noncopyable {
  using socket = typename T::socket;
  using endpoint = typename T::endpoint;

 public:
  tcp_channel_t(socket& socket, const PackOption& pack_option, const uint32_t& max_body_size)
      : socket_(socket), pack_option_(pack_option), max_body_size_(max_body_size) {
    asio_net_LOGD("tcp_channel: %p", this);
    if (pack_option == PackOption::DISABLE) {
      max_body_size_ = std::min<uint32_t>(1024, max_body_size_);
      max_body_size_ = std::max<uint32_t>(32, max_body_size_);
    }
  }

  ~tcp_channel_t() {
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

  endpoint local_endpoint() {
    return socket_.local_endpoint();
  }

  endpoint remote_endpoint() {
    return socket_.remote_endpoint();
  }

 public:
  std::function<void()> on_close;
  std::function<void(std::string)> on_data;

 protected:
  void do_read_start(std::shared_ptr<tcp_channel_t> self = nullptr) {
    if (pack_option_ == PackOption::ENABLE) {
      do_read_header(std::move(self));
    } else {
      // init read_msg_.body as buffer
      read_msg_.body.resize(max_body_size_);
      do_read_data(std::move(self));
    }
  }

 private:
  void do_read_header(std::shared_ptr<tcp_channel_t> self) {
    asio::async_read(socket_, asio::buffer(&read_msg_.length, sizeof(read_msg_.length)),
                     [this, self = std::move(self)](const std::error_code& ec, std::size_t /*length*/) mutable {
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

  void do_read_body(std::shared_ptr<tcp_channel_t> self) {
    read_msg_.body.resize(read_msg_.length);
    asio::async_read(socket_, asio::buffer(read_msg_.body), [this, self = std::move(self)](const std::error_code& ec, std::size_t) mutable {
      if (!ec) {
        auto msg = std::move(read_msg_.body);
        read_msg_.clear();
        if (on_data) on_data(std::move(msg));
        do_read_header(std::move(self));
      } else {
        do_close();
      }
    });
  }

  void do_read_data(std::shared_ptr<tcp_channel_t> self) {
    auto& readBuffer = read_msg_.body;
    socket_.async_read_some(asio::buffer(readBuffer), [this, self = std::move(self)](const std::error_code& ec, std::size_t length) mutable {
      if (!ec) {
        if (on_data) on_data(std::string(read_msg_.body.data(), length));
        do_read_data(std::move(self));
      } else {
        do_close();
      }
    });
  }

  void do_write(std::string msg) {
    auto keeper = std::make_unique<detail::message>(std::move(msg));
    if (pack_option_ == PackOption::ENABLE && keeper->length > max_body_size_) {
      asio_net_LOGE("write: body size=%u > max_body_size=%u", keeper->length, max_body_size_);
      do_close();
    }

    auto buffer = {
        asio::buffer(&keeper->length, sizeof(keeper->length)),
        asio::buffer(keeper->body),
    };
    asio::async_write(socket_, buffer, [this, keeper = std::move(keeper)](const std::error_code& ec, std::size_t /*length*/) {
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
  socket& socket_;
  const PackOption& pack_option_;
  uint32_t max_body_size_;
  detail::message read_msg_;
};

}  // namespace detail
}  // namespace asio_net
