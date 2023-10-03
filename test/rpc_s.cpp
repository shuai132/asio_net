#include <cstdio>

#include "asio_net/rpc_server.hpp"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main() {
  asio::io_context context;
  rpc_server server(context, PORT);
  server.on_session = [](const std::weak_ptr<rpc_session>& rs) {
    auto session = rs.lock();
    LOG("on_session: %p", session.get());
    session->on_close = [rs] {
      LOG("session on_close: %p", rs.lock().get());
    };
    session->rpc->subscribe("cmd", [](const std::string& data) -> std::string {
      LOG("session on cmd: %s", data.c_str());
      return "world";
    });
  };
  server.start(true);
  return 0;
}
