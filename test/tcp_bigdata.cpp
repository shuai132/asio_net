#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "assert_def.h"
#include "log.h"
#include "tcp_client.hpp"
#include "tcp_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  // server
  std::thread([] {
    asio::io_context context;
    tcp_server server(context, PORT, Config{.auto_pack = true});
    server.on_session = [&](const std::weak_ptr<tcp_session>& ws) {
      LOG("on_session:");
      auto session = ws.lock();
      session->on_close = [] {
        LOG("session on_close:");
      };
      session->on_data = [ws, &context](const std::string& data) {
        ASSERT(!ws.expired());
        LOG("session on_data: %s", data.c_str());
        static int count = 0;
        if (++count == 3) {
          context.stop();
        }
      };
    };
    server.start(true);
  }).detach();

  // client
  std::thread([] {
    asio::io_context context;
    tcp_client client(context, Config{.auto_pack = true});
    client.on_open = [&] {
      LOG("client on_open:");
      std::string msg("hello");
      msg.resize(1024 * 1024 * 100);
      LOG("send...");
      client.send(msg);
      LOG("send...");
      client.send(msg);
      LOG("send...");
      client.send(msg);
    };
    client.on_data = [&](const std::string& data) {
      LOG("client on_data:");
    };
    client.on_close = [&] {
      LOG("client on_close:");
    };
    client.open("localhost", PORT);
    context.run();
  }).join();
  return EXIT_SUCCESS;
}
