#include "asio_net/udp_client.hpp"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  udp_client client(context);
  udp_client::endpoint endpoint(asio::ip::address_v4::from_string("127.0.0.1"), PORT);
  client.send_to("hello", endpoint, [&](const std::error_code& ec, std::size_t size) {
    LOG("result: ec: %d, size: %lu", ec.value(), size);
  });
  context.run();
  return EXIT_SUCCESS;
}
