#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "asio_net/udp_client.hpp"
#include "asio_net/udp_server.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  static uint32_t test_count_max = 10000;
  static std::atomic_uint32_t test_count_received;
  if (argc >= 2) {
    test_count_max = std::strtol(argv[1], nullptr, 10);
  }

  std::thread([] {
    asio::io_context context;
    udp_server server(context, PORT);
    server.on_data = [](uint8_t* data, size_t size, const udp_server::endpoint& from) {
      (void)data;
      (void)size;
      (void)from;
#ifndef ASIO_NET_DISABLE_ON_DATA_PRINT
      LOG("on_data: %s", std::string((char*)data, size).c_str());
#endif
      test_count_received++;
    };
    server.start();
  }).detach();

  std::atomic_uint32_t send_failed_count{0};
  std::thread([&] {
    asio::io_context context;
    udp_client client(context);
    context.post([&] {
      udp_client::endpoint endpoint(asio::ip::address_v4::from_string("127.0.0.1"), PORT);
      for (uint32_t i = 0; i < test_count_max; ++i) {
        auto data = std::to_string(i);
        client.send_to(data, endpoint, [&](const std::error_code& ec, std::size_t size) {
          (void)size;
          if (ec) {
            send_failed_count++;
          }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
    context.run();
  }).join();

  std::this_thread::sleep_for(std::chrono::seconds(1));
  LOG("test_count_max: %d", test_count_max);
  LOG("test_count_received: %d", test_count_received.load());
  LOG("send_failed_count: %d", send_failed_count.load());
  LOG("lost: %f%%", 100 * (double)(test_count_max - test_count_received) / test_count_max);
  ASSERT(test_count_max - test_count_received == send_failed_count);
  return EXIT_SUCCESS;
}
