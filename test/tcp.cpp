#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "asio_net/tcp_client.hpp"
#include "asio_net/tcp_server.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  uint32_t test_count_max = 10000;
  uint32_t test_count_expect = 0;
  if (argc >= 2) {
    const auto count = std::strtol(argv[1], nullptr, 10);
    ASSERT(count > 0 && static_cast<unsigned long>(count) <= UINT32_MAX);
    test_count_max = static_cast<uint32_t>(count);
  }

  asio::io_context context;
  tcp_config config;
  config.auto_pack = true;
  // Bind before opening the client, and keep both sides alive until they close.
  tcp_server server(context, PORT, config);
  tcp_client client(context, config);
  bool pass_flag_session_close = false;
  bool pass_flag_client_close = false;
  bool timed_out = false;
  const auto finish_if_closed = [&] {
    if (pass_flag_session_close && pass_flag_client_close) context.stop();
  };

  asio::steady_timer deadline(context, std::chrono::seconds(10));
  deadline.async_wait([&](const std::error_code& ec) {
    if (ec) return;
    timed_out = true;
    std::fprintf(stderr, "TCP timeout: received=%u/%u, client_closed=%d, server_closed=%d\n",
                 test_count_expect, test_count_max, pass_flag_client_close, pass_flag_session_close);
    context.stop();
  });

  server.on_session = [&](const std::weak_ptr<tcp_session>& ws) {
    LOG("on_session:");
    auto session = ws.lock();
    ASSERT(session);
    session->on_close = [&] {
      LOG("session on_close:");
      pass_flag_session_close = true;
      finish_if_closed();
    };
    session->on_data = [ws](std::string data) {
      auto session = ws.lock();
      ASSERT(session);
#ifndef ASIO_NET_DISABLE_ON_DATA_PRINT
      LOG("session on_data: %s", data.c_str());
#endif
      ASSERT(session->send(std::move(data)));
    };
  };
  server.start();

  client.on_open = [&] {
    LOG("client on_open:");
    ASSERT(client.is_open);
    for (uint32_t i = 0; i < test_count_max; ++i) {
      ASSERT(client.send(std::to_string(i)));
    }
  };
  client.on_data = [&](const std::string& data) {
    ASSERT(client.is_open);
#ifndef ASIO_NET_DISABLE_ON_DATA_PRINT
    LOG("client on_data: %s", data.c_str());
#endif
    ASSERT(std::to_string(test_count_expect++) == data);
    if (test_count_expect == test_count_max) client.close();
  };
  client.on_close = [&] {
    ASSERT(!client.is_open);
    pass_flag_client_close = true;
    ASSERT(test_count_expect == test_count_max);
    LOG("client on_close:");
    finish_if_closed();
  };
  client.on_open_failed = [&](const std::error_code& ec) {
    std::fprintf(stderr, "TCP open failed: %s\n", ec.message().c_str());
    ASSERT(!ec);
  };
  client.open("127.0.0.1", PORT);
  context.run();

  ASSERT(!timed_out);
  ASSERT(test_count_expect == test_count_max);
  ASSERT(pass_flag_session_close);
  ASSERT(pass_flag_client_close);
  return EXIT_SUCCESS;
}
