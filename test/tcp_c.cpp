#include "asio_net/tcp_client.hpp"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main() {
  asio::io_context context;
  tcp_client client(context);
  client.on_open = [&] {
    LOG("client on_open:");
    client.send("hello");
  };
  client.on_data = [](const std::string& data) {
    LOG("client on_data: %s", data.c_str());
  };
  client.on_open_failed = [](std::error_code ec) {
    LOG("client on_open_failed: %d, %s", ec.value(), ec.message().c_str());
  };
  client.on_close = [] {
    LOG("client on_close:");
  };
  client.open("localhost", PORT);
  client.run();
  return 0;
}
