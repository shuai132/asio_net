#include <cstdio>

#include "asio_net/rpc_client.hpp"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main() {
  asio::io_context context;
  rpc_client client(context);
  client.on_open = [&](const std::shared_ptr<rpc_core::rpc>& rpc) {
    LOG("client on_open:");
    rpc->cmd("cmd")
        ->msg(std::string("hello"))
        ->rsp([](const std::string& data) {
          LOG("cmd rsp: %s", data.c_str());
        })
        ->call();
  };
  client.on_close = [] {
    LOG("client on_close:");
  };

  auto test = [&] {
    LOG("open...");
    client.open("localhost", PORT);
    client.close();
  };
  std::function<void()> time_task;
  asio::steady_timer timer(context);
  time_task = [&] {
    timer.expires_after(std::chrono::milliseconds(100));
    timer.async_wait([&](std::error_code ec) {
      (void)ec;
      test();
      time_task();
    });
  };
  time_task();

  client.run();
  return 0;
}
