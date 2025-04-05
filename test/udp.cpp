#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "asio_net/udp_client.hpp"
#include "asio_net/udp_server.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  // Keep assertion diagnostics visible when stdout is redirected by CI.
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  uint32_t test_count_max = 1000;
  if (argc >= 2) {
    char* end = nullptr;
    const auto count = std::strtol(argv[1], &end, 10);
    ASSERT(end != argv[1] && *end == '\0');
    ASSERT(count > 0 && static_cast<unsigned long>(count) <= (std::numeric_limits<uint32_t>::max)());
    test_count_max = static_cast<uint32_t>(count);
  }

  asio::io_context context;
  // Bind before sending so thread scheduling cannot lose the first datagrams.
  udp_server server(context, PORT);
  udp_client client(context);
  const udp_client::endpoint endpoint(asio::ip::address_v4::loopback(), PORT);
  uint32_t test_count_received = 0;
  uint32_t send_completed_count = 0;
  bool timed_out = false;

  asio::steady_timer deadline(context, std::chrono::seconds(10));
  deadline.async_wait([&](const std::error_code& ec) {
    if (ec) return;
    timed_out = true;
    std::fprintf(stderr, "UDP timeout: received=%u, send_completed=%u, expected=%u\n",
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
      if (ec) std::fprintf(stderr, "UDP send failed: %s\n", ec.message().c_str());
      ASSERT(!ec);
      ASSERT(size == expected_size);
      ++send_completed_count;
      finish_if_complete();
    });
  };
  server.on_data = [&](uint8_t* data, size_t size, const udp_server::endpoint& from) {
    ASSERT(from.address() == endpoint.address());
    ASSERT(std::string(reinterpret_cast<char*>(data), size) == std::to_string(test_count_received));
    ++test_count_received;
    // A send completion does not guarantee delivery. Keep only one datagram in
    // flight, advancing on receipt rather than flooding the UDP receive queue.
    if (test_count_received < test_count_max) send_next();
    finish_if_complete();
  };

  send_next();
  context.run();

  LOG("test_count_max: %u", test_count_max);
  LOG("test_count_received: %u", test_count_received);
  LOG("send_completed_count: %u", send_completed_count);
  ASSERT(!timed_out);
  ASSERT(test_count_received == test_count_max);
  ASSERT(send_completed_count == test_count_max);
  return EXIT_SUCCESS;
}
