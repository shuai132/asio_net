#pragma once

#include "rpc_core.hpp"

namespace asio_net {
namespace dds {

using rpc_s = std::shared_ptr<rpc_core::rpc>;
using rpc_w = std::weak_ptr<rpc_core::rpc>;
using handle_t = std::function<void(std::string msg)>;
using handle_s = std::shared_ptr<handle_t>;

struct Msg {
  std::string topic;
  std::string data;
  RPC_CORE_DEFINE_TYPE_INNER(topic, data);
};

}  // namespace dds
}  // namespace asio_net
