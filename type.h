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
    if (auto_pack) {
      max_body_size = std::min<uint32_t>(1024, max_body_size);
      max_body_size = std::max<uint32_t>(32, max_body_size);
    }
  }
};

}  // namespace asio_net
