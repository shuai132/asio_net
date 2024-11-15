#pragma once

#include <deque>
#include <utility>

#include "asio.hpp"
#include "asio_net/detail/log.h"
#include "asio_net/detail/message.hpp"
#include "config.hpp"
#include "detail/noncopyable.hpp"

namespace asio_net {

class serial_port : detail::noncopyable {
 public:
  explicit serial_port(asio::io_context& io_context, serial_config config = {})
      : io_context_(io_context), serial_(io_context), config_(std::move(config)) {}

  template <typename Option>
  inline void set_option(const Option& option) {
    serial_.set_option(option);
  }

  template <typename Option>
  inline void get_option(Option& option) {
    serial_.get_option(option);
  }

  void open(std::string device = {}) {
    if (!device.empty()) {
      config_.device = std::move(device);
    }
    try_open();
  }

  bool is_open() const {
    return serial_.is_open();
  }

  void send(std::string msg) {
    do_write(std::move(msg), false);
  }

  void close() {
    cancel_reconnect();
    do_close();
  }

  void set_reconnect(uint32_t ms) {
    reconnect_ms_ = ms;
    reconnect_timer_ = std::make_unique<asio::steady_timer>(io_context_);
  }

  void cancel_reconnect() {
    if (reconnect_timer_) {
      reconnect_timer_->cancel();
      reconnect_timer_ = nullptr;
    }
  }

  serial_config& config() {
    return config_;
  }

  void run() {
    auto work = asio::make_work_guard(io_context_);
    io_context_.run();
  }

 private:
  void try_open() {
    if (on_try_open) on_try_open();
    try {
      serial_.open(config_.device);
      if (on_open) on_open();
      read_msg_.resize(config_.max_recv_buffer_size);
      do_read_data();
    } catch (const std::system_error& e) {
      if (on_open_failed) on_open_failed(e.code());
      check_reconnect();
    }
  }

  void do_write(std::string msg, bool from_queue) {
    if (msg.size() > config_.max_send_buffer_size) {
      ASIO_NET_LOGE("msg size=%zu > max_send_buffer_size=%u", msg.size(), config_.max_send_buffer_size);
      do_close();
    }

    // block wait send_buffer idle
    while (msg.size() + send_buffer_now_ > config_.max_send_buffer_size) {
      ASIO_NET_LOGV("block wait send_buffer idle");
      io_context_.run_one();
    }

    // queue for asio::async_write
    if (!from_queue && send_buffer_now_ != 0) {
      ASIO_NET_LOGV("queue for asio::async_write");
      send_buffer_now_ += (uint32_t)msg.size();
      write_msg_queue_.emplace_back(std::move(msg));
      return;
    }
    if (!from_queue) {
      send_buffer_now_ += (uint32_t)msg.size();
    }

    auto keeper = std::make_unique<detail::message>(std::move(msg));
    auto buffer = asio::buffer(keeper->body);
    asio::async_write(serial_, buffer, [this, keeper = std::move(keeper)](const std::error_code& ec, std::size_t /*length*/) {
      send_buffer_now_ -= (uint32_t)keeper->body.size();
      if (ec) {
        do_close();
      }

      if (!write_msg_queue_.empty()) {
        asio::post(io_context_, [this, msg = std::move(write_msg_queue_.front())]() mutable {
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
    check_reconnect();

    if (!is_open()) return;
    asio::error_code ec;
    serial_.close(ec);
    if (ec) {
      ASIO_NET_LOGE("do_close: %s", ec.message().c_str());
    }
    if (on_close) on_close();
  }

  void reset_data() {
    read_msg_.clear();
    read_msg_.shrink_to_fit();
    send_buffer_now_ = 0;
    write_msg_queue_.clear();
    write_msg_queue_.shrink_to_fit();
  }

  void do_read_data() {
    serial_.async_read_some(asio::buffer(read_msg_), [this](const std::error_code& ec, std::size_t length) mutable {
      if (!ec) {
        if (on_data) on_data(std::string(read_msg_.data(), length));
        do_read_data();
      } else {
        do_close();
      }
    });
  }

  void check_reconnect() {
    if (reconnect_timer_) {
      reconnect_timer_->expires_after(std::chrono::milliseconds(reconnect_ms_));
      reconnect_timer_->async_wait([this](std::error_code ec) {
        if (!ec) {
          try_open();
        }
      });
    }
  }

 public:
  std::function<void()> on_try_open;
  std::function<void()> on_open;
  std::function<void(std::error_code)> on_open_failed;
  std::function<void()> on_close;

  std::function<void(std::string)> on_data;

 private:
  asio::io_context& io_context_;
  asio::serial_port serial_;
  serial_config config_;

  std::string read_msg_;
  uint32_t send_buffer_now_ = 0;
  std::deque<std::string> write_msg_queue_;

  std::unique_ptr<asio::steady_timer> reconnect_timer_;
  uint32_t reconnect_ms_ = 0;
};

}  // namespace asio_net
