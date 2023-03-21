#pragma once

#include <utility>

#include "RpcCore.hpp"
#include "detail/noncopyable.hpp"
#include "detail/tcp_channel_t.hpp"

namespace asio_net {

template <typename T>
class rpc_session_t : noncopyable, public std::enable_shared_from_this<rpc_session_t<T>> {
 public:
  explicit rpc_session_t(asio::io_context& io_context) : io_context_(io_context) {
    asio_net_LOGD("rpc_session: %p", this);
  }

  ~rpc_session_t() {
    asio_net_LOGD("~rpc_session: %p", this);
  }

 public:
  void init(std::weak_ptr<detail::tcp_channel_t<T>> ws) {
    tcp_session_ = std::move(ws);
    auto tcp_session = tcp_session_.lock();

    rpc = RpcCore::Rpc::create();

    rpc->setTimer([this](uint32_t ms, RpcCore::Rpc::TimeoutCb cb) {
      auto timer = std::make_shared<asio::steady_timer>(io_context_);
      timer->expires_after(std::chrono::milliseconds(ms));
      auto tp = timer.get();
      tp->async_wait([timer = std::move(timer), cb = std::move(cb)](const std::error_code&) {
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

    // bind rpc_session lifecycle to tcp_session and end with on_close
    tcp_session->on_close = [rpc_session = this->shared_from_this()]() mutable {
      if (rpc_session->on_close) {
        rpc_session->on_close();
      }
      rpc_session = nullptr;
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
  std::weak_ptr<detail::tcp_channel_t<T>> tcp_session_;
};

}  // namespace asio_net
