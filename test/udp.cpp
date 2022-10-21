#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "udp_client.hpp"
#include "udp_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  static uint32_t test_count_max = 100000;
  static std::atomic_uint32_t test_count_received;
  if (argc >= 2) {
    test_count_max = std::atol(argv[1]);
  }

  std::thread([] {
    asio::io_context context;
    udp_server server(context, PORT);
    server.on_data = [](uint8_t* data, size_t size, const udp::endpoint& from) {
#ifndef asio_net_DISABLE_ON_DATA_PRINT
      printf("on_data: %s\n", std::string((char*)data, size).c_str());
#endif
      test_count_received++;
    };
    context.run();
  }).detach();
  std::thread([] {
    asio::io_context context;
    udp_client client(context);
    context.post([&client] {
      auto endpoint = udp::endpoint(asio::ip::address_v4::from_string("127.0.0.1"), PORT);
      for (uint32_t i = 0; i < test_count_max; ++i) {
        auto data = std::to_string(i);
        client.send_to(data, endpoint);
        usleep(1);
      }
    });
    context.run();
  }).join();
  sleep(1);
  printf("lost: %f%%\n", 100 * (double)(test_count_max - test_count_received) / test_count_max);
  return EXIT_SUCCESS;
}
