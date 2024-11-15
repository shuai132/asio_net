#include <thread>

#include "asio_net/rpc_client.hpp"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main() {
  asio::io_context context;
  std::shared_ptr<rpc_client> client;

  std::thread([&] {
    asio::post(context, [&] {
      client = std::make_shared<rpc_client>(context);
      client->on_open = [&](const std::shared_ptr<rpc_core::rpc>& rpc) {
        LOG("client on_open:");
        rpc->cmd("cmd")
            ->msg(std::string("hello"))
            ->rsp([](const std::string& data) {
              LOG("cmd rsp: %s", data.c_str());
            })
            ->call();
      };
      client->on_open_failed = [](std::error_code ec) {
        LOG("client on_open_failed: %d, %s", ec.value(), ec.message().c_str());
      };
      client->on_close = [] {
        LOG("client on_close:");
      };
      client->open("localhost", PORT);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    asio::post(context, [&] {
      client = nullptr;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    context.stop();
  }).detach();

  auto work = asio::make_work_guard(context);
  context.run();
  return 0;
}
