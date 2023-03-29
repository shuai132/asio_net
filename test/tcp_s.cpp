#include <cstdio>

#include "assert_def.h"
#include "log.h"
#include "tcp_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  tcp_server server(context, PORT);
  server.on_session = [](const std::weak_ptr<tcp_session>& ws) {
    LOG("on_session:");
    auto session = ws.lock();
    session->on_close = [] {
      LOG("session on_close:");
    };
    session->on_data = [ws](std::string data) {
      ASSERT(!ws.expired());
      LOG("session on_data: %zu, %s", data.size(), data.c_str());
      ws.lock()->send(std::move(data));
    };
  };
  server.start(true);
  return 0;
}
