#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "udp_client.hpp"
#include "udp_server.hpp"

using namespace asio_net;

const char* ENDPOINT = "/tmp/foobar";

int main(int argc, char** argv) {
  ::unlink(ENDPOINT);  // remove previous binding

  static uint32_t test_count_max = 100000;
  static std::atomic_uint32_t test_count_received;
  if (argc >= 2) {
    test_count_max = std::atol(argv[1]);
  }

  std::thread([] {
    asio::io_context context;
    domain_udp_server server(context, ENDPOINT);
    server.on_data = [](uint8_t* data, size_t size, const domain_udp_server::endpoint& from) {
#ifndef asio_net_DISABLE_ON_DATA_PRINT
      printf("on_data: %s\n", std::string((char*)data, size).c_str());
#endif
      test_count_received++;
    };
    context.run();
  }).detach();

  // wait domain create
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::thread([] {
    asio::io_context context;
    domain_udp_client client(context, ENDPOINT);
    context.post([&client] {
      for (uint32_t i = 0; i < test_count_max; ++i) {
        auto data = std::to_string(i);
        client.send(data);
        usleep(1);
      }
    });
    context.run();
  }).join();
  sleep(1);
  printf("lost: %f%%\n", 100 * (double)(test_count_max - test_count_received) / test_count_max);
  return EXIT_SUCCESS;
}
