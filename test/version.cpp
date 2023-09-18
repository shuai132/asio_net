#include "asio_net.hpp"
#include "log.h"

int main(int argc, char** argv) {
  LOGI("asio: %d", ASIO_VERSION);
  LOGI("rpc_core: %d", RPC_CORE_VERSION);
  LOGI("asio_net: %d", ASIO_NET_VERSION);
  return 0;
}
