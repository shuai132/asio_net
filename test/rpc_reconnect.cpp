#include <cstdio>
#include <cstdlib>
#include <thread>

#include "asio_net/rpc_client.hpp"
#include "asio_net/rpc_server.hpp"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main() {
  // server
  std::thread([] {
    asio::io_context context;
    rpc_server server(context, PORT);  // static for test session lifecycle
    server.on_session = [](const std::weak_ptr<rpc_session>& rs) {
      LOG("on_session:");
      auto session = rs.lock();
      session->close();
    };
    server.start(true);
  }).detach();

  // client
  std::thread([] {
    asio::io_context context;
    rpc_client client(context);  // static for test session lifecycle
    client.on_open = [](const std::shared_ptr<rpc_core::rpc>& rpc) {
      (void)rpc;
      LOG("client on_open:");
    };
    client.on_close = [&] {
      LOG("client on_close:");
      static int count = 0;
      if (++count == 3) {
        client.stop();
      }
    };
    client.set_reconnect(1);
    client.open("localhost", PORT);
    client.run();
  }).join();
  return EXIT_SUCCESS;
}
