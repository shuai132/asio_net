#include "log.h"
#include "udp_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  udp_server server(context, PORT);
  server.on_data = [](uint8_t* data, size_t size, const udp_server::endpoint& from) {
    LOG("on_data: %s, from: %s, port: %d", std::string((char*)data, size).c_str(), from.address().to_string().c_str(), from.port());
  };
  server.start();
  return EXIT_SUCCESS;
}
