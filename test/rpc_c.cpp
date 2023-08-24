#include <cstdio>

#include "log.h"
#include "rpc_client.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  rpc_client client(context);
  client.on_open = [&](const std::shared_ptr<rpc_core::rpc>& rpc) {
    LOG("client on_open:");
    rpc->cmd("cmd")
        ->msg(std::string("hello"))
        ->rsp([&](const std::string& data) {
          LOG("cmd rsp: %s", data.c_str());
        })
        ->call();
  };
  client.on_close = [] {
    LOG("client on_close:");
  };
  client.open("localhost", PORT);
  client.run();
  return 0;
}
