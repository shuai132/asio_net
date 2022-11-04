#pragma once

#include <utility>

#include "RpcCore.hpp"
#include "detail/noncopyable.hpp"
#include "tcp_server.hpp"

namespace asio_net {

class rpc_session : noncopyable, public std::enable_shared_from_this<rpc_session> {
 public:
  explicit rpc_session(asio::io_context& io_context) : io_context_(io_context) {}

 public:
  void init(std::weak_ptr<tcp_session> ws) {
    tcp_session_ = std::move(ws);
    auto tcp_session = tcp_session_.lock();

    rpc = RpcCore::Rpc::create();

    rpc->setTimer([this](uint32_t ms, RpcCore::Rpc::TimeoutCb cb) {
      auto timer = std::make_shared<asio::steady_timer>(io_context_);
      timer->expires_after(std::chrono::milliseconds(ms));
      timer->async_wait([timer = std::move(timer), cb = std::move(cb)](const std::error_code&) {
        cb();
      });
    });

    rpc->getConn()->sendPackageImpl = [this](std::string data) {
      auto tcp_session = tcp_session_.lock();
      if (tcp_session) {
        tcp_session->send(std::move(data));
      } else {
        asio_net_LOGW("tcp_session expired on sendPackage");
      }
    };

    // bind rpc_session lifecycle to tcp_session
    tcp_session->on_close = [rpc_session = shared_from_this()] {
      if (rpc_session->on_close) {
        rpc_session->on_close();
      }
    };

    tcp_session->on_data = [this](std::string data) {
      rpc->getConn()->onRecvPackage(std::move(data));
    };
  }

  void close() {
    auto ts = tcp_session_.lock();
    if (ts) {
      ts->close();
    }
  }

 public:
  std::function<void()> on_close;

 public:
  std::shared_ptr<RpcCore::Rpc> rpc;

 private:
  asio::io_context& io_context_;
  std::weak_ptr<tcp_session> tcp_session_;
};

}  // namespace asio_net
