#pragma once

#include <cstdint>

#include "rpc_core/rpc.hpp"

namespace asio_net {

struct tcp_config {
  bool auto_pack = false;
  bool enable_ipv6 = false;
  uint32_t max_body_size = UINT32_MAX;
  uint32_t max_send_buffer_size = UINT32_MAX;

  // socket option
  uint32_t socket_send_buffer_size = UINT32_MAX;
  uint32_t socket_recv_buffer_size = UINT32_MAX;

  void init() {
    // when auto_pack disable, max_body_size means buffer size, default is 1024 bytes
    if ((!auto_pack) && (max_body_size == UINT32_MAX)) {
      max_body_size = 1024;
    }
  }
};

struct rpc_config {
  // rpc config
  std::shared_ptr<rpc_core::rpc> rpc;  // NOTICE: set on server means that a single connection is desired
  uint32_t ping_interval_ms = 0;       // 0: disable auto ping, >0: enable auto ping
  uint32_t pong_timeout_ms = 5000;     // if pong timeout, client/session will be close

  // socket config
  bool enable_ipv6 = false;
  uint32_t max_body_size = UINT32_MAX;
  uint32_t max_send_buffer_size = UINT32_MAX;

  // socket option
  uint32_t socket_send_buffer_size = UINT32_MAX;
  uint32_t socket_recv_buffer_size = UINT32_MAX;

  tcp_config to_tcp_config() {
    return {.auto_pack = true,
            .enable_ipv6 = enable_ipv6,
            .max_body_size = max_body_size,
            .max_send_buffer_size = max_send_buffer_size,
            .socket_send_buffer_size = socket_send_buffer_size,
            .socket_recv_buffer_size = socket_recv_buffer_size};
  }
};

struct serial_config {
  // device
  std::string device;

  // buffer
  uint32_t max_send_buffer_size = UINT32_MAX;
  uint32_t max_recv_buffer_size = 1024;
};

}  // namespace asio_net
