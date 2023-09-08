#pragma once

#include "noncopyable.hpp"
#include "tcp_channel_t.hpp"

namespace asio_net {
namespace detail {

template <typename T>
class tcp_client_t : public tcp_channel_t<T> {
  using socket = typename T::socket;
  using endpoint = typename T::endpoint;

 public:
  explicit tcp_client_t(asio::io_context& io_context, config config = {})
      : tcp_channel_t<T>(socket_, config_), io_context_(io_context), socket_(io_context), config_(config) {
    config_.init();
  }

  /**
   * connect to server
   *
   * @param host A string identifying a location. May be a descriptive name or a numeric address string.
   * @param port The port to open.
   */
  void open(std::string host, uint16_t port) {
    open_ = [this, host = std::move(host), port] {
      do_open(host, port);
    };
    open_();
  }

  /**
   * connect to domain socket
   *
   * @param endpoint e.g. /tmp/foobar
   */
  void open(std::string endpoint) {
    open_ = [this, endpoint = std::move(endpoint)] {
      do_open(endpoint);
    };
    open_();
  }

  void close() {
    cancel_reconnect();
    tcp_channel_t<T>::close();
  }

  void set_reconnect(uint32_t ms) {
    reconnect_ms_ = ms;
    reconnect_timer_ = std::make_unique<asio::steady_timer>(socket_.get_executor());
  }

  void cancel_reconnect() {
    if (reconnect_timer_) {
      reconnect_timer_->cancel();
      reconnect_timer_ = nullptr;
    }
  }

  void check_reconnect() {
    if (!is_open && reconnect_timer_) {
      reconnect_timer_->expires_after(std::chrono::milliseconds(reconnect_ms_));
      reconnect_timer_->async_wait([this](const asio::error_code& ec) {
        if (is_open) return;
        if (!ec) {
          open_();
        }
      });
    }
  }

  void run() {
    asio::io_context::work work(io_context_);
    io_context_.run();
  }

  void stop() {
    close();
    io_context_.stop();
  }

 private:
  void do_open(const std::string& host, uint16_t port) {
    static_assert(std::is_same<asio::ip::tcp, T>::value, "");
    auto resolver = std::make_unique<typename T::resolver>(socket_.get_executor());
    auto rp = resolver.get();
    rp->async_resolve(typename T::resolver::query(host, std::to_string(port)),
                      [this, resolver = std::move(resolver)](const std::error_code& ec, const typename T::resolver::results_type& endpoints) {
                        if (!ec) {
                          asio::async_connect(socket_, endpoints, [this](const std::error_code& ec, const endpoint&) {
                            async_connect_handler(ec);
                          });
                        } else {
                          if (on_open_failed) on_open_failed(ec);
                          check_reconnect();
                        }
                      });
  }

  void do_open(const std::string& endpoint) {
    static_assert(std::is_same<T, asio::local::stream_protocol>::value, "");
    socket_.async_connect(typename T::endpoint(endpoint), [this](const std::error_code& ec) {
      async_connect_handler(ec);
    });
  }

  void async_connect_handler(const std::error_code& ec) {
    if (!ec) {
      this->init_socket();
      tcp_channel_t<T>::on_close = [this] {
        tcp_client_t::on_close();
        check_reconnect();
      };
      if (on_open) on_open();
      if (reconnect_timer_) {
        reconnect_timer_->cancel();
      }
      this->do_read_start();
    } else {
      if (on_open_failed) on_open_failed(ec);
      check_reconnect();
    }
  }

 public:
  std::function<void()> on_open;
  std::function<void(std::error_code)> on_open_failed;
  std::function<void()> on_close;

 public:
  bool is_open = false;

 private:
  asio::io_context& io_context_;
  socket socket_;
  config config_;
  std::unique_ptr<asio::steady_timer> reconnect_timer_;
  uint32_t reconnect_ms_ = 0;
  std::function<void()> open_;
};

}  // namespace detail
}  // namespace asio_net
