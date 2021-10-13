#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "assert_def.h"
#include "tcp_client.hpp"
#include "tcp_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  static uint32_t test_count_max = 100000;
  static uint32_t test_count_expect = 0;
  if (argc >= 2) {
    test_count_max = std::atol(argv[1]);
  }

  static std::atomic_bool pass_flag_session_close{false};
  static std::atomic_bool pass_flag_client_close{false};
  std::thread([] {
    asio::io_context context;
    tcp_server server(context, PORT);
    server.on_session = [](const std::weak_ptr<tcp_session>& ws) {
      printf("on_session:\n");
      auto session = ws.lock();
      session->on_close = [] {
        printf("session on_close:\n");
        pass_flag_session_close = true;
      };
      session->on_data = [ws](std::string data) {
        ASSERT(!ws.expired());
        printf("session on_data: %s\n", data.c_str());
        ws.lock()->send(std::move(data));
      };
    };
    context.run();
  }).detach();
  std::thread([] {
    asio::io_context context;
    tcp_client client(context);
    client.on_open = [&] {
      printf("client on_open:\n");
      for (uint32_t i = 0; i < test_count_max; ++i) {
        client.send(std::to_string(i));
      }
    };
    client.on_data = [&](const std::string& data) {
      printf("client on_data: %s\n", data.c_str());
      ASSERT(std::to_string(test_count_expect++) == data);
      if (test_count_expect == test_count_max - 1) {
        client.close();
      }
    };
    client.on_close = [&] {
      pass_flag_client_close = true;
      ASSERT(test_count_expect == test_count_max - 1);
      printf("client on_close:\n");
      context.stop();
    };
    client.open("localhost", std::to_string(PORT));
    context.run();
  }).join();
  ASSERT(pass_flag_session_close);
  ASSERT(pass_flag_client_close);
  return EXIT_SUCCESS;
}
