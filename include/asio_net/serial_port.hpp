#pragma once

#include <deque>

#include "asio.hpp"
#include "asio_net/detail/log.h"
#include "asio_net/detail/message.hpp"
#include "config.hpp"
#include "detail/noncopyable.hpp"

namespace asio_net {

class serial_port : detail::noncopyable {
 public:
  explicit serial_port(asio::io_context& io_context, std::string device = {}, uint32_t baud_rate = 115200)
      : io_context_(io_context), serial_(io_context), device_(std::move(device)), baud_rate_(baud_rate) {}
  explicit serial_port(asio::io_context& io_context, std::string vid, std::string pid)
      : io_context_(io_context), serial_(io_context), vid_(std::move(vid)), pid_(std::move(pid)) {}

  template <typename SettableSerialPortOption>
  void set_option(const SettableSerialPortOption& option) {
    serial_.set_option(option);
  }

  void open(std::string device = {}) {
    if (!device.empty()) {
      device_ = std::move(device);
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

  void run() {
    asio::io_context::work work(io_context_);
    io_context_.run();
  }

 private:
  void init_options() {
    serial_.set_option(asio::serial_port::baud_rate(baud_rate_));
    serial_.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
    serial_.set_option(asio::serial_port::parity(asio::serial_port::parity::none));
    serial_.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
    serial_.set_option(asio::serial_port::character_size(asio::serial_port::character_size(8)));
  }

  void try_open() {
    try {
      serial_.open(device_);
      init_options();
      on_open();
      read_msg_.resize(config_.max_recv_buffer_size);
      do_read_data();
    } catch (const std::system_error& e) {
      on_open_failed(e.code());
      check_reconnect();
    }
  }

  void do_write(std::string msg, bool from_queue) {
    // block wait send_buffer idle
    while (msg.size() + send_buffer_now_ > config_.max_send_buffer_size) {
      ASIO_NET_LOGV("block wait send_buffer idle");
      io_context_.run_one();
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
    buffer.emplace_back(asio::buffer(keeper->body));
    asio::async_write(serial_, buffer, [this, keeper = std::move(keeper)](const std::error_code& ec, std::size_t /*length*/) {
      send_buffer_now_ -= keeper->body.size();
      if (ec) {
        do_close();
      }

      if (!write_msg_queue_.empty()) {
        io_context_.post([this, msg = std::move(write_msg_queue_.front())]() mutable {
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
      ASIO_NET_LOGW("do_close: %s", ec.message().c_str());
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
  std::function<void()> on_open;
  std::function<void(std::error_code)> on_open_failed;
  std::function<void()> on_close;

  std::function<void(std::string)> on_data;

 private:
  asio::io_context& io_context_;
  asio::serial_port serial_;
  std::string device_;
  uint32_t baud_rate_ = 115200;
  std::string vid_;
  std::string pid_;

  serial_config config_;
  std::string read_msg_;
  uint32_t send_buffer_now_ = 0;
  std::deque<std::string> write_msg_queue_;

  std::unique_ptr<asio::steady_timer> reconnect_timer_;
  uint32_t reconnect_ms_ = 0;
};

}  // namespace asio_net
