#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "log.h"
#include "rpc_client.hpp"
#include "rpc_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  // server
  std::thread([] {
    asio::io_context context;
    static rpc_server server(context, PORT);  // static for test session lifecycle
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
    static rpc_client client(context);  // static for test session lifecycle
    client.on_open = [&](const std::shared_ptr<RpcCore::Rpc>& rpc) {
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
