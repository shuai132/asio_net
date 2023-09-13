#pragma once

#include <utility>

#include "asio.hpp"
#include "rpc_session.hpp"
#include "tcp_server.hpp"

namespace asio_net {
namespace detail {

template <typename T>
class rpc_server_t : noncopyable {
 public:
  rpc_server_t(asio::io_context& io_context, uint16_t port, bool enable_ipv6 = false, uint32_t max_body_size = UINT32_MAX)
      : io_context_(io_context), server_(io_context, port, config{.auto_pack = true, .enable_ipv6 = enable_ipv6, .max_body_size = max_body_size}) {
    static_assert(std::is_same<T, asio::ip::tcp>::value, "");
    server_.on_session = [this](std::weak_ptr<detail::tcp_session_t<T>> ws) {
      auto session = std::make_shared<rpc_session_t<T>>(io_context_);
      session->init(std::move(ws));
      if (on_session) {
        on_session(session);
      }
    };
  }

  rpc_server_t(asio::io_context& io_context, const std::string& endpoint, bool enable_ipv6 = false, uint32_t max_body_size = UINT32_MAX)
      : io_context_(io_context),
        server_(io_context, endpoint, config{.auto_pack = true, .enable_ipv6 = enable_ipv6, .max_body_size = max_body_size}) {
    static_assert(std::is_same<T, asio::local::stream_protocol>::value, "");
    server_.on_session = [this](std::weak_ptr<detail::tcp_session_t<T>> ws) {
      auto session = std::make_shared<rpc_session_t<T>>(io_context_);
      session->init(std::move(ws));
      if (on_session) {
        on_session(session);
      }
    };
  }

 public:
  void start(bool loop = false) {
    server_.start(loop);
  }

 public:
  std::function<void(std::weak_ptr<rpc_session_t<T>>)> on_session;

 private:
  asio::io_context& io_context_;
  detail::tcp_server_t<T> server_;
};

}  // namespace detail
}  // namespace asio_net
