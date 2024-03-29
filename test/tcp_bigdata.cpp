#include <cstdio>
#include <cstdlib>
#include <thread>

#include "asio_net/tcp_client.hpp"
#include "asio_net/tcp_server.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main() {
  // server
  std::thread([] {
    asio::io_context context;
    tcp_server server(context, PORT, tcp_config{.auto_pack = true});
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
    tcp_client client(context, tcp_config{.auto_pack = true});
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
      (void)data;
      LOG("client on_data:");
    };
    client.on_close = [&] {
      LOG("client on_close:");
      client.stop();
    };
    client.open("localhost", PORT);
    client.run();
  }).join();
  return EXIT_SUCCESS;
}
