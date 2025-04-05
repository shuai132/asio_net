#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "asio_net/udp_client.hpp"
#include "asio_net/udp_server.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

int main(int argc, char** argv) {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  uint32_t test_count_max = 10000;
  if (argc >= 2) {
    char* end = nullptr;
    const auto count = std::strtol(argv[1], &end, 10);
    ASSERT(end != argv[1] && *end == '\0');
    ASSERT(count > 0 && static_cast<unsigned long>(count) <= (std::numeric_limits<uint32_t>::max)());
    test_count_max = static_cast<uint32_t>(count);
  }

  const auto path = std::string("asio-net-domain-udp-") +
                    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sock";
  uint32_t test_count_received = 0;
  uint32_t send_completed_count = 0;
  bool timed_out = false;
  {
    asio::io_context context;
    // Bind before sending, and use one event loop instead of detached threads
    // and a startup sleep. A unique path also isolates simultaneous test runs.
    domain_udp_server server(context, path);
    domain_udp_client client(context);
    const domain_udp_client::endpoint endpoint(path);
    asio::steady_timer deadline(context, std::chrono::seconds(10));
    deadline.async_wait([&](const std::error_code& ec) {
      if (ec) return;
      timed_out = true;
      std::fprintf(stderr, "Domain UDP timeout: received=%u, send_completed=%u, expected=%u\n",
                   test_count_received, send_completed_count, test_count_max);
      context.stop();
    });
    const auto finish_if_complete = [&] {
      if (test_count_received == test_count_max && send_completed_count == test_count_max) {
        context.stop();
      }
    };
    const auto send_next = [&] {
      const auto data = std::to_string(test_count_received);
      client.send_to(data, endpoint, [&, expected_size = data.size()](const std::error_code& ec, std::size_t size) {
        if (ec) std::fprintf(stderr, "Domain UDP send failed: %s\n", ec.message().c_str());
        ASSERT(!ec);
        ASSERT(size == expected_size);
        ++send_completed_count;
        finish_if_complete();
      });
    };
    server.on_data = [&](uint8_t* data, size_t size, const domain_udp_server::endpoint&) {
      ASSERT(std::string(reinterpret_cast<char*>(data), size) == std::to_string(test_count_received));
      ++test_count_received;
      // Advance on receipt, so the local datagram queue cannot be flooded.
      if (test_count_received < test_count_max) send_next();
      finish_if_complete();
    };
    send_next();
    context.run();
  }
  ASSERT(std::remove(path.c_str()) == 0);
  LOG("test_count_max: %u", test_count_max);
  LOG("test_count_received: %u", test_count_received);
  LOG("send_completed_count: %u", send_completed_count);
  ASSERT(!timed_out);
  ASSERT(test_count_received == test_count_max);
  ASSERT(send_completed_count == test_count_max);
  return EXIT_SUCCESS;
}
