#pragma once

#include <utility>

#include "noncopyable.hpp"
#include "rpc_core.hpp"
#include "rpc_session_t.hpp"
#include "tcp_client_t.hpp"

namespace asio_net {
namespace detail {

template <socket_type T>
class rpc_client_t : noncopyable {
 public:
  explicit rpc_client_t(asio::io_context& io_context, rpc_config rpc_config = {})
      : io_context_(io_context), rpc_config_(rpc_config), client_(std::make_shared<detail::tcp_client_t<T>>(io_context, rpc_config.to_tcp_config())) {
    client_->on_open = [this]() {
      auto session = std::make_shared<rpc_session_t<T>>(io_context_, rpc_config_);
      rpc_session_ = session;
      session->init(client_);

      session->on_close = [this] {
        client_->on_data = nullptr;
        if (on_close) on_close();
        client_->check_reconnect();
      };

      start_ping();
      if (on_open) on_open(session->rpc);
    };

    client_->on_open_failed = [this](const std::error_code& ec) {
      if (on_open_failed) on_open_failed(ec);
    };
  }

#ifdef ASIO_NET_ENABLE_SSL
  explicit rpc_client_t(asio::io_context& io_context, asio::ssl::context& ssl_context, rpc_config rpc_config = {})
      : io_context_(io_context), client_(std::make_shared<detail::tcp_client_t<T>>(io_context, ssl_context, rpc_config.to_tcp_config())) {
    client_->on_open = [this]() {
      auto session = std::make_shared<rpc_session_t<T>>(io_context_, rpc_config_);
      rpc_session_ = session;
      session->init(client_);

      session->on_close = [this] {
        client_->on_data = nullptr;
        if (on_close) on_close();
        client_->check_reconnect();
      };

      start_ping();
      if (on_open) on_open(session->rpc);
    };

    client_->on_open_failed = [this](const std::error_code& ec) {
      if (on_open_failed) on_open_failed(ec);
    };
  }
#endif

  void open(std::string ip, uint16_t port) {
    static_assert(T == socket_type::normal || T == socket_type::ssl, "");
    client_->open(std::move(ip), port);
  }

  void open(std::string endpoint) {
    static_assert(T == socket_type::domain, "");
    client_->open(std::move(endpoint));
  }

  void close() {
    stop_ping();
    client_->close();
  }

  void set_reconnect(uint32_t ms) {
    client_->set_reconnect(ms);
  }

  void cancel_reconnect() {
    client_->cancel_reconnect();
  }

  rpc_config& config() {
    return rpc_config_;
  }

  void start_ping() {
    if (rpc_config_.ping_interval_ms == 0) return;
    if (!ping_timer_) {
      ping_timer_ = std::make_unique<asio::steady_timer>(io_context_);
    }
    ping_timer_->expires_after(std::chrono::milliseconds(rpc_config_.ping_interval_ms));
    ping_timer_->async_wait([this](std::error_code ec) {
      if (ec) return;
      auto session = rpc_session_.lock();
      if (session && session->rpc->is_ready()) {
        ASIO_NET_LOGD("ping...");
        session->rpc->ping()
            ->rsp([this] {
              start_ping();
            })
            ->timeout([this]() {
              ASIO_NET_LOGW("ping timeout");
              stop_ping();
              client_->close(true);
            })
            ->timeout_ms(rpc_config_.pong_timeout_ms)
            ->call();
      }
    });
  }

  void stop_ping() {
    if (ping_timer_) {
      ping_timer_->cancel();
      ping_timer_ = nullptr;
    }
  }

  void run() {
    client_->run();
  }

  void stop() {
    client_->stop();
  }

 public:
  std::function<void(std::shared_ptr<rpc_core::rpc>)> on_open;
  std::function<void()> on_close;
  std::function<void(std::error_code)> on_open_failed;

 private:
  asio::io_context& io_context_;
  rpc_config rpc_config_;
  std::shared_ptr<detail::tcp_client_t<T>> client_;
  std::weak_ptr<rpc_session_t<T>> rpc_session_;
  std::unique_ptr<asio::steady_timer> ping_timer_;
};

}  // namespace detail
}  // namespace asio_net
