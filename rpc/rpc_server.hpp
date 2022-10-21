#include <utility>

#include "asio.hpp"
#include "rpc_session.hpp"
#include "tcp_server.hpp"

namespace asio_net {

class rpc_server : noncopyable {
 public:
  rpc_server(asio::io_context& io_context, uint16_t port, uint32_t max_body_size_ = UINT32_MAX)
      : io_context_(io_context), server_(io_context, port, PackOption::ENABLE, max_body_size_) {
    server_.on_session = [this](const std::weak_ptr<tcp_session>& ws) {
      auto rpc_session = std::make_shared<asio_net::rpc_session>(ws);
      auto rpc = RpcCore::Rpc::create();
      rpc_session->rpc = rpc;

      rpc->setTimer([this](uint32_t ms, RpcCore::Rpc::TimeoutCb cb) {
        auto timer = std::make_shared<asio::steady_timer>(io_context_);
        timer->expires_after(std::chrono::milliseconds(ms));
        timer->async_wait([timer = std::move(timer), cb = std::move(cb)](asio::error_code) {
          cb();
        });
      });
      rpc->getConn()->sendPackageImpl = [ws](std::string data) {
        ws.lock()->send(std::move(data));
      };

      auto session = ws.lock();
      session->on_close = [rpc_session] {
        if (rpc_session->on_close) {
          rpc_session->on_close();
        }
      };
      session->on_data = [rpc_session](std::string data) {
        rpc_session->rpc->getConn()->onRecvPackage(std::move(data));
      };

      if (on_session) {
        on_session(rpc_session);
      }
    };
  }

 public:
  void start(bool loop = false) {
    server_.start(loop);
  }

 public:
  std::function<void(std::weak_ptr<rpc_session>)> on_session;

 private:
  asio::io_context& io_context_;
  tcp_server server_;
};

}  // namespace asio_net
