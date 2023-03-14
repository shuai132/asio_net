#pragma once

#include <utility>

#include "RpcCore.hpp"
#include "detail/noncopyable.hpp"
#include "rpc_session.hpp"
#include "tcp_client.hpp"

namespace asio_net {

template <typename T>
class rpc_client_t : noncopyable {
 public:
  explicit rpc_client_t(asio::io_context& io_context, uint32_t max_body_size = UINT32_MAX)
      : io_context_(io_context), client_(std::make_shared<detail::tcp_client_t<T>>(io_context, PackOption::ENABLE, max_body_size)) {
    client_->on_open = [this]() {
      auto session = std::make_shared<rpc_session_t<T>>(io_context_);
      session->init(client_);

      session->on_close = [this] {
        client_->on_data = nullptr;
        if (on_close) on_close();
      };

      if (on_open) on_open(session->rpc);
    };

    client_->on_open_failed = [this](const std::error_code& ec) {
      if (on_open_failed) on_open_failed(ec);
    };
  }

  void open(const std::string& ip, uint16_t port) {
    static_assert(std::is_same<T, asio::ip::tcp>::value, "");
    client_->open(ip, port);
  }

  void open(const std::string& endpoint) {
    static_assert(std::is_same<T, asio::local::stream_protocol>::value, "");
    client_->open(endpoint);
  }

  void close() {
    client_->close();
  }

 public:
  std::function<void(std::shared_ptr<RpcCore::Rpc>)> on_open;
  std::function<void()> on_close;
  std::function<void(std::error_code)> on_open_failed;

 private:
  asio::io_context& io_context_;
  std::shared_ptr<detail::tcp_client_t<T>> client_;
};

}  // namespace asio_net
