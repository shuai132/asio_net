#include "asio_net/dds.hpp"

#include <cstdio>
#include <cstdlib>
#include <thread>

#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

static void init_server() {
  std::thread([] {
    asio::io_context context;
    dds_server server(context, PORT);
    server.start(true);
  }).detach();
}

static void init_client() {
  for (int i = 0; i < 3; ++i) {
    std::thread([i] {
      asio::io_context context;
      dds_client client(context);
      client.subscribe("topic_all", [=](const std::string& data) {
        LOG("client_%d: topic:%s, data:%s", i, "topic_all", data.c_str());
      });
      std::string topic_tmp = "topic_" + std::to_string(i);
      client.subscribe(topic_tmp, [=](const std::string& data) {
        LOG("client_%d: topic:%s, data:%s", i, topic_tmp.c_str(), data.c_str());
      });
      client.open("localhost", PORT);
      client.run();
    }).detach();
  }
}

int main() {
  init_server();
  init_client();

  asio::io_context context;
  dds_client client(context);
  client.open("localhost", PORT);

  std::function<void()> time_task;
  asio::steady_timer timer(context);
  time_task = [&] {
    timer.expires_after(std::chrono::seconds(2));
    timer.async_wait([&](std::error_code ec) {
      (void)ec;
      client.publish("topic_all", "hello");
      client.publish("topic_0", "to client_0");
      client.publish("topic_1", "to client_1");
      client.publish("topic_2", "to client_2");
      time_task();
    });
  };
  time_task();

  client.run();
  return 0;
}
