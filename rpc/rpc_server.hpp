#pragma once

#include <utility>

#include "asio.hpp"
#include "rpc_session.hpp"
#include "tcp_server.hpp"

namespace asio_net {

class rpc_server : noncopyable {
 public:
  rpc_server(asio::io_context& io_context, uint16_t port, uint32_t max_body_size_ = UINT32_MAX)
      : io_context_(io_context), server_(io_context, port, PackOption::ENABLE, max_body_size_) {
    server_.on_session = [this](std::weak_ptr<tcp_session> ws) {
      auto session = std::make_shared<rpc_session>(io_context_);
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
  std::function<void(std::weak_ptr<rpc_session>)> on_session;

 private:
  asio::io_context& io_context_;
  tcp_server server_;
};

}  // namespace asio_net
