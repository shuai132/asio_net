#include "asio_net/dds.hpp"

#include <cstdio>
#include <cstdlib>
#include <thread>

#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

static std::atomic_bool received_flag[3]{};
static std::atomic_int received_all_cnt{0};

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
      client.subscribe("topic_all", [i](const std::string& data) {
        LOG("client_%d: topic:%s, data:%s", i, "topic_all", data.c_str());
        ++received_all_cnt;
      });
      std::string topic_tmp = "topic_" + std::to_string(i);
      client.subscribe(topic_tmp, [i, topic_tmp](const std::string& data) {
        LOG("client_%d: topic:%s, data:%s", i, topic_tmp.c_str(), data.c_str());
        ASSERT(data == "to client_" + std::to_string(i));
        ASSERT(!received_flag[i]);
        received_flag[i] = true;
      });
      client.open("localhost", PORT);
      client.run();
    }).detach();
  }
}

static const int interval_ms = 1000;
static void interval_check(dds_client& client) {
  /// 1. test basic
  static bool first_run = true;
  static std::atomic_bool received_flag_self{false};
  if (first_run) {
    first_run = false;
    client.subscribe("topic_self", [](const std::string& msg) {
      LOG("received: topic_self: %s", msg.c_str());
      ASSERT(!received_flag_self);
      received_flag_self = true;
    });
  } else {
    // check and reset flag
    for (auto& flag : received_flag) {
      ASSERT(flag);
      flag = false;
    }

    ASSERT(received_all_cnt == 3);
    received_all_cnt = 0;

    ASSERT(received_flag_self);
    received_flag_self = false;
  }

  client.publish("topic_self", "to client_self");
  client.publish("topic_0", "to client_0");
  client.publish("topic_1", "to client_1");
  client.publish("topic_2", "to client_2");
  client.publish("topic_all", "hello");

  /// 2.1 test unsubscribe
  client.subscribe("topic_test_0", [](const std::string& msg) {
    (void)(msg);
    ASSERT(false);
  });
  client.unsubscribe("topic_test_0");
  client.publish("topic_test_0");

  /// 2.2
  auto id = client.subscribe("topic_test_1", [](const std::string& msg) {
    (void)(msg);
    ASSERT(false);
  });
  client.unsubscribe(id);
  client.publish("topic_test_1");
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
    timer.expires_after(std::chrono::milliseconds(interval_ms));
    timer.async_wait([&](std::error_code ec) {
      (void)ec;
      interval_check(client);
      time_task();
    });
  };
  time_task();

  client.run();
  return 0;
}
