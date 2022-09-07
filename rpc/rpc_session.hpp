#pragma once

#include "RpcCore.hpp"
#include "noncopyable.hpp"

namespace asio_net {

class rpc_session : noncopyable {
 public:
  explicit rpc_session() = default;

 public:
  std::function<void()> on_close;

 public:
  std::shared_ptr<RpcCore::Rpc> rpc;
};

}  // namespace asio_net
