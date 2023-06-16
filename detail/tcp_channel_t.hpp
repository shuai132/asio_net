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
  tcp_channel_t(socket& socket, const Config& config) : socket_(socket), config_(config) {
    asio_net_LOGD("tcp_channel: %p", this);
  }

  ~tcp_channel_t() {
    asio_net_LOGD("~tcp_channel: %p", this);
  }

  void init_socket() {
    if (config_.max_send_buffer_size != UINT32_MAX) {
      asio::socket_base::send_buffer_size option(config_.max_send_buffer_size);
      socket_.set_option(option);
    }
    if (config_.max_recv_buffer_size != UINT32_MAX) {
      asio::socket_base::receive_buffer_size option(config_.max_send_buffer_size);
      socket_.set_option(option);
    }
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
    if (config_.auto_pack) {
      do_read_header(std::move(self));
    } else {
      // init read_msg_.body as buffer
      read_msg_.body.resize(config_.max_body_size);
      do_read_data(std::move(self));
    }
  }

 private:
  void do_read_header(std::shared_ptr<tcp_channel_t> self) {
    asio::async_read(socket_, asio::buffer(&read_msg_.length, sizeof(read_msg_.length)),
                     [this, self = std::move(self)](const std::error_code& ec, std::size_t /*length*/) mutable {
                       if (ec) {
                         do_close();
                         return;
                       }
                       if (read_msg_.length <= config_.max_body_size) {
                         do_read_body(std::move(self));
                       } else {
                         asio_net_LOGE("read: body size=%u > max_body_size=%u", read_msg_.length, config_.max_body_size);
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
    if (config_.auto_pack && keeper->length > config_.max_body_size) {
      asio_net_LOGE("write: body size=%u > max_body_size=%u", keeper->length, config_.max_body_size);
      do_close();
    }

    if (keeper->length > config_.max_send_buffer_size) {
      asio_net_LOGE("write: body size=%u > max_send_buffer_size=%u", keeper->length, config_.max_send_buffer_size);
      do_close();
    }

    // block wait send_buffer idle
    while (keeper->length + send_buffer_now_ > config_.max_send_buffer_size) {
      static_cast<asio::io_context*>(&socket_.get_executor().context())->run_one();
    }

    std::vector<asio::const_buffer> buffer;
    if (config_.auto_pack) {
      buffer.emplace_back(&keeper->length, sizeof(keeper->length));
    }
    buffer.emplace_back(asio::buffer(keeper->body));
    send_buffer_now_ += keeper->body.size();
    asio::async_write(socket_, buffer, [this, keeper = std::move(keeper)](const std::error_code& ec, std::size_t /*length*/) {
      send_buffer_now_ -= keeper->body.size();
      if (ec) {
        do_close();
      }
    });
  }

  void do_close() {
    if (!is_open()) return;
    asio::error_code ec;
    socket_.close(ec);
    if (ec) {
      asio_net_LOGW("do_close: %s", ec.message().c_str());
    }
    if (on_close) on_close();
  }

 private:
  socket& socket_;
  const Config& config_;
  detail::message read_msg_;
  uint32_t send_buffer_now_ = 0;
};

}  // namespace detail
}  // namespace asio_net
