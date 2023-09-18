#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "asio_net/tcp_client.hpp"
#include "asio_net/tcp_server.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const char* ENDPOINT = "/tmp/foobar";

int main(int argc, char** argv) {
  ::unlink(ENDPOINT);  // remove previous binding

  static uint32_t test_count_max = 10000;
  static uint32_t test_count_expect = 0;
  if (argc >= 2) {
    test_count_max = std::strtol(argv[1], nullptr, 10);
  }

  static std::atomic_bool pass_flag_session_close{false};
  static std::atomic_bool pass_flag_client_close{false};
  std::thread([] {
    asio::io_context context;
    domain_tcp_server server(context, ENDPOINT, tcp_config{.auto_pack = true});
    server.on_session = [](const std::weak_ptr<domain_tcp_session>& ws) {
      LOG("on_session:");
      auto session = ws.lock();
      session->on_close = [] {
        LOG("session on_close:");
        pass_flag_session_close = true;
      };
      session->on_data = [ws](std::string data) {
        ASSERT(!ws.expired());
#ifndef ASIO_NET_DISABLE_ON_DATA_PRINT
        LOG("session on_data: %s", data.c_str());
#endif
        ws.lock()->send(std::move(data));
      };
    };
    server.start(true);
  }).detach();

  // wait domain create
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::thread([] {
    asio::io_context context;
    domain_tcp_client client(context, tcp_config{.auto_pack = true});
    client.on_open = [&] {
      LOG("client on_open:");
      for (uint32_t i = 0; i < test_count_max; ++i) {
        client.send(std::to_string(i));
      }
    };
    client.on_data = [&](const std::string& data) {
#ifndef ASIO_NET_DISABLE_ON_DATA_PRINT
      LOG("client on_data: %s", data.c_str());
#endif
      ASSERT(std::to_string(test_count_expect++) == data);
      if (test_count_expect == test_count_max - 1) {
        client.close();
      }
    };
    client.on_close = [&] {
      pass_flag_client_close = true;
      ASSERT(test_count_expect == test_count_max - 1);
      LOG("client on_close:");
      client.stop();
    };
    client.open(ENDPOINT);
    client.run();
  }).join();
  ASSERT(pass_flag_session_close);
  ASSERT(pass_flag_client_close);
  return EXIT_SUCCESS;
}
