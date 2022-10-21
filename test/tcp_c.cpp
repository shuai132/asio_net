#include <cstdio>

#include "tcp_client.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  tcp_client client(context);
  client.on_open = [&] {
    printf("client on_open:\n");
    client.send("hello");
  };
  client.on_data = [&](const std::string& data) {
    printf("client on_data: %s\n", data.c_str());
    client.close();
  };
  client.on_close = [&] {
    printf("client on_close:\n");
    context.stop();
  };
  client.open("localhost", std::to_string(PORT));
  context.run();
  return 0;
}
