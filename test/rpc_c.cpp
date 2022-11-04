#include <cstdio>

#include "rpc_client.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  rpc_client client(context);
  client.on_open = [&](const std::shared_ptr<RpcCore::Rpc>& rpc) {
    printf("client on_open:\n");
    rpc->cmd("cmd")
        ->msg(RpcCore::String("hello"))
        ->rsp([&](const RpcCore::String& data) {
          printf("cmd rsp: %s\n", data.c_str());
        })
        ->call();
  };
  client.on_close = [&] {
    printf("client on_close:\n");
    context.stop();
  };
  client.open("localhost", PORT);
  context.run();
  return 0;
}
