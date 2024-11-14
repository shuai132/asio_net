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
    init();
  }

#ifdef ASIO_NET_ENABLE_SSL
  explicit rpc_client_t(asio::io_context& io_context, asio::ssl::context& ssl_context, rpc_config rpc_config = {})
      : io_context_(io_context),
        rpc_config_(rpc_config),
        client_(std::make_shared<detail::tcp_client_t<T>>(io_context, ssl_context, rpc_config.to_tcp_config())) {
    init();
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

  void run() {
    client_->run();
  }

  void stop() {
    client_->stop();
  }

 private:
  void init() {
    client_->on_open = [this]() {
      auto session = std::make_shared<rpc_session_t<T>>(io_context_, rpc_config_);
      rpc_session_ = session;
      session->init(client_);

      session->on_close = [this] {
        client_->on_data = nullptr;
        if (on_close) on_close();
        client_->check_reconnect();
      };

      session->start_ping();
      if (on_open) on_open(session->rpc);
    };

    client_->on_open_failed = [this](const std::error_code& ec) {
      if (on_open_failed) on_open_failed(ec);
    };
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
};

}  // namespace detail
}  // namespace asio_net
