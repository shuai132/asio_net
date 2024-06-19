#pragma once

#include <deque>
#include <utility>

#include "../config.hpp"
#include "asio.hpp"
#include "log.h"
#include "message.hpp"
#include "noncopyable.hpp"
#include "socket_type.hpp"

namespace asio_net {
namespace detail {

template <socket_type T>
class tcp_channel_t : private noncopyable {
 public:
  tcp_channel_t(typename socket_impl<T>::socket& socket, const tcp_config& config) : socket_(socket), config_(config) {
    ASIO_NET_LOGD("tcp_channel: %p", this);
  }

  ~tcp_channel_t() {
    ASIO_NET_LOGD("~tcp_channel: %p", this);
  }

 protected:
  void init_socket() {
    if (config_.socket_send_buffer_size != UINT32_MAX) {
      asio::socket_base::send_buffer_size option(config_.socket_send_buffer_size);
      get_socket().set_option(option);
    }
    if (config_.socket_recv_buffer_size != UINT32_MAX) {
      asio::socket_base::receive_buffer_size option(config_.socket_recv_buffer_size);
      get_socket().set_option(option);
    }
  }

  inline auto& get_socket() const {
    return socket_.lowest_layer();
  }

 public:
  /**
   * async send message
   * 1. will close if error occur, e.g. msg.size() > max_body_size
   * 2. will block wait if send buffer > max_send_buffer_size
   * 3. not threadsafe, only can be used on io_context thread
   *
   * @param msg can be string or binary
   */
  void send(std::string msg) {
    do_write(std::move(msg), false);
  }

  /**
   * close socket
   * will trigger @see`on_close` if opened
   */
  void close() {
    do_close();
  }

  bool is_open() const {
    return get_socket().is_open();
  }

  typename socket_impl<T>::endpoint local_endpoint() {
    return socket_.local_endpoint();
  }

  typename socket_impl<T>::endpoint remote_endpoint() {
    return socket_.remote_endpoint();
  }

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
    asio::async_read(
        socket_, asio::buffer(&read_msg_.length, sizeof(read_msg_.length)),
        [this, self = std::move(self), alive = std::weak_ptr<void>(this->is_alive_)](const std::error_code& ec, std::size_t size) mutable {
          if (alive.expired()) return;

          if (ec == asio::error::eof || ec == asio::error::connection_reset) {
            do_close();
            return;
          } else if (ec || size == 0) {
            ASIO_NET_LOGD("do_read_header: %s, size: %zu", ec.message().c_str(), size);
            do_close();
            return;
          }

          if (read_msg_.length <= config_.max_body_size) {
            do_read_body(std::move(self));
          } else {
            ASIO_NET_LOGE("read: body size=%u > max_body_size=%u", read_msg_.length, config_.max_body_size);
            do_close();
          }
        });
  }

  void do_read_body(std::shared_ptr<tcp_channel_t> self) {
    read_msg_.body.resize(read_msg_.length);
    asio::async_read(
        socket_, asio::buffer(read_msg_.body),
        [this, self = std::move(self), alive = std::weak_ptr<void>(this->is_alive_)](const std::error_code& ec, std::size_t size) mutable {
          if (alive.expired()) return;

          if (ec == asio::error::eof || ec == asio::error::connection_reset) {
            do_close();
            return;
          } else if (ec || size == 0) {
            ASIO_NET_LOGD("do_read_body: %s, size: %zu", ec.message().c_str(), size);
            do_close();
            return;
          }

          auto msg = std::move(read_msg_.body);
          read_msg_.clear();
          if (on_data) on_data(std::move(msg));
          do_read_header(std::move(self));
        });
  }

  void do_read_data(std::shared_ptr<tcp_channel_t> self) {
    auto& readBuffer = read_msg_.body;
    socket_.async_read_some(asio::buffer(readBuffer), [this, self = std::move(self), alive = std::weak_ptr<void>(this->is_alive_)](
                                                          const std::error_code& ec, std::size_t length) mutable {
      if (alive.expired()) return;
      if (!ec) {
        if (on_data) on_data(std::string(read_msg_.body.data(), length));
        do_read_data(std::move(self));
      } else {
        do_close();
      }
    });
  }

  void do_write(std::string msg, bool from_queue) {
    if (config_.auto_pack && msg.size() > config_.max_body_size) {
      ASIO_NET_LOGE("write: body size=%zu > max_body_size=%u", msg.size(), config_.max_body_size);
      do_close();
    }

    if (msg.size() > config_.max_send_buffer_size) {
      ASIO_NET_LOGE("write: body size=%zu > max_send_buffer_size=%u", msg.size(), config_.max_send_buffer_size);
      do_close();
    }

    // block wait send_buffer idle
    while (msg.size() + send_buffer_now_ > config_.max_send_buffer_size) {
      ASIO_NET_LOGV("block wait send_buffer idle");
      static_cast<asio::io_context*>(&socket_.get_executor().context())->run_one();
    }

    // queue for asio::async_write
    if (!from_queue && send_buffer_now_ != 0) {
      ASIO_NET_LOGV("queue for asio::async_write");
      send_buffer_now_ += msg.size();
      write_msg_queue_.emplace_back(std::move(msg));
      return;
    }
    if (!from_queue) {
      send_buffer_now_ += msg.size();
    }

    auto keeper = std::make_unique<detail::message>(std::move(msg));
    std::vector<asio::const_buffer> buffer;
    if (config_.auto_pack) {
      buffer.emplace_back(&keeper->length, sizeof(keeper->length));
    }
    buffer.emplace_back(asio::buffer(keeper->body));
    asio::async_write(
        socket_, buffer,
        [this, keeper = std::move(keeper), alive = std::weak_ptr<void>(this->is_alive_)](const std::error_code& ec, std::size_t /*length*/) {
          if (alive.expired()) return;
          send_buffer_now_ -= keeper->body.size();
          if (ec) {
            do_close();
          }

          if (!write_msg_queue_.empty()) {
            asio::post(socket_.get_executor(), [this, msg = std::move(write_msg_queue_.front())]() mutable {
              do_write(std::move(msg), true);
            });
            write_msg_queue_.pop_front();
          } else {
            write_msg_queue_.shrink_to_fit();
          }
        });
  }

  void do_close() {
    reset_data();

    if (!is_open()) return;
    asio::error_code ec;
    get_socket().close(ec);
    if (ec) {
      ASIO_NET_LOGE("do_close: %s", ec.message().c_str());
    }
    if (on_close) on_close();
  }

  void reset_data() {
    read_msg_.clear();
    send_buffer_now_ = 0;
    write_msg_queue_.clear();
    write_msg_queue_.shrink_to_fit();
  }

 public:
  std::function<void()> on_close;
  std::function<void(std::string)> on_data;

 protected:
  std::shared_ptr<void> is_alive_ = std::make_shared<uint8_t>();

 private:
  typename socket_impl<T>::socket& socket_;
  const tcp_config& config_;
  detail::message read_msg_;
  size_t send_buffer_now_ = 0;
  std::deque<std::string> write_msg_queue_;
};

}  // namespace detail
}  // namespace asio_net
