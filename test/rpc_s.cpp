#include <cstdio>

#include "rpc_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  rpc_server server(context, PORT);
  server.on_session = [](const std::weak_ptr<rpc_session>& rs) {
    printf("on_session:\n");
    auto session = rs.lock();
    session->on_close = [] {
      printf("session on_close:\n");
    };
    session->rpc->subscribe<RpcCore::String, RpcCore::String>("cmd", [](const RpcCore::String& data) {
      printf("session on cmd: %s\n", data.c_str());
      return "world";
    });
  };
  server.start(true);
  return 0;
}
