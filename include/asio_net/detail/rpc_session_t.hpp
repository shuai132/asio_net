#pragma once

#include <utility>

#include "noncopyable.hpp"
#include "rpc_core.hpp"
#include "tcp_channel_t.hpp"

namespace asio_net {
namespace detail {

template <socket_type T>
class rpc_session_t : noncopyable, public std::enable_shared_from_this<rpc_session_t<T>> {
 public:
  explicit rpc_session_t(asio::io_context& io_context, rpc_config& rpc_config) : io_context_(io_context), rpc_config_(rpc_config) {
    ASIO_NET_LOGD("rpc_session: %p", this);
  }

  ~rpc_session_t() {
    ASIO_NET_LOGD("~rpc_session: %p", this);
  }

 public:
  bool init(std::weak_ptr<detail::tcp_channel_t<T>> ws) {
    tcp_session_ = std::move(ws);
    auto tcp_session = tcp_session_.lock();

    if (rpc_config_.rpc) {
      if (rpc_config_.rpc->is_ready()) {
        ASIO_NET_LOGD("rpc already connected");
        tcp_session->close();
        return false;
      }
      rpc = rpc_config_.rpc;
    } else {
      rpc = rpc_core::rpc::create();
    }

    rpc->set_timer([this](uint32_t ms, rpc_core::rpc::timeout_cb cb) {
      auto timer = std::make_shared<asio::steady_timer>(io_context_);
      timer->expires_after(std::chrono::milliseconds(ms));
      auto tp = timer.get();
      tp->async_wait([timer = std::move(timer), cb = std::move(cb)](const std::error_code&) {
        cb();
      });
    });

    rpc->get_connection()->send_package_impl = [this](std::string data) {
      auto tcp_session = tcp_session_.lock();
      if (tcp_session) {
        tcp_session->send(std::move(data));
      } else {
        ASIO_NET_LOGW("tcp_session expired");
      }
    };

    rpc->set_ready(true);

    // bind rpc_session lifecycle to tcp_session and end with on_close
    tcp_session->on_close = [this, rpc_session = this->shared_from_this()]() mutable {
      rpc->set_ready(false);

      stop_ping();

      if (rpc_session->on_close) {
        rpc_session->on_close();
      }
      // post delay destroy rpc_session, ensure rpc.rsp() callback finish
      asio::post(io_context_, [rpc_session = std::move(rpc_session)] {});
      // clear tcp_session->on_close, avoid called more than once by close api
      tcp_session_.lock()->on_close = nullptr;
    };

    tcp_session->on_data = [this](std::string data) {
      rpc->get_connection()->on_recv_package(std::move(data));
    };

    start_ping();
    return true;
  }

  void close() {
    auto ts = tcp_session_.lock();
    if (ts) {
      ts->close();
    }
  }

  void start_ping() {
    if (rpc_config_.ping_interval_ms == 0) return;
    if (!ping_timer_) {
      ping_timer_ = std::make_unique<asio::steady_timer>(io_context_);
    }
    ping_timer_->expires_after(std::chrono::milliseconds(rpc_config_.ping_interval_ms));
    ping_timer_->async_wait([ws = std::weak_ptr<rpc_session_t<T>>(rpc_session_t<T>::shared_from_this())](std::error_code ec) {
      if (ec) return;
      auto session = ws.lock();
      if (session && session->rpc->is_ready()) {
        ASIO_NET_LOGD("ping...");
        session->rpc->ping()
            ->rsp([ws] {
              auto session = ws.lock();
              if (session) {
                session->start_ping();
              }
            })
            ->timeout([ws]() {
              ASIO_NET_LOGW("ping timeout");
              auto session = ws.lock();
              if (session) {
                session->stop_ping();
                session->close();
              }
            })
            ->timeout_ms(session->rpc_config_.pong_timeout_ms)
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

 public:
  std::function<void()> on_close;

 public:
  std::shared_ptr<rpc_core::rpc> rpc;

 private:
  asio::io_context& io_context_;
  rpc_config& rpc_config_;
  std::weak_ptr<detail::tcp_channel_t<T>> tcp_session_;
  std::unique_ptr<asio::steady_timer> ping_timer_;
};

}  // namespace detail
}  // namespace asio_net
