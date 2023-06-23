#pragma once

#include <algorithm>
#include <cstdint>

namespace asio_net {

struct Config {
  bool auto_pack = false;
  uint32_t max_body_size = UINT32_MAX;
  uint32_t max_send_buffer_size = UINT32_MAX;
  uint32_t max_recv_buffer_size = UINT32_MAX;

  void init() {
    // when auto_pack disable, max_body_size means buffer size, default is 1024 bytes
    if ((!auto_pack) && (max_body_size == UINT32_MAX)) {
      max_body_size = 1024;
    }
  }
};

}  // namespace asio_net
