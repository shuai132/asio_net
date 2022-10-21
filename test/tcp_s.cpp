#include <cstdio>

#include "assert_def.h"
#include "tcp_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  tcp_server server(context, PORT);
  server.on_session = [](const std::weak_ptr<tcp_session>& ws) {
    printf("on_session:\n");
    auto session = ws.lock();
    session->on_close = [] {
      printf("session on_close:\n");
    };
    session->on_data = [ws](std::string data) {
      ASSERT(!ws.expired());
      printf("session on_data: %zu, %s\n", data.size(), data.c_str());
      ws.lock()->send(std::move(data));
    };
  };
  server.start(true);
  return 0;
}
