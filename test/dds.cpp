#include <cstdio>
#include <cstdlib>
#include <thread>

#include "asio_net.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

#ifdef TEST_DDS_DOMAIN
const char* ENDPOINT = "/tmp/foobar";
#elif defined(TEST_DDS_NORMAL)
const uint16_t PORT = 6666;
#elif defined(TEST_DDS_SSL)
const uint16_t PORT = 6667;
#endif

static int interval_ms = 1000;
static bool run_once = false;

static std::atomic_bool received_flag[3]{};
static std::atomic_int received_all_cnt{0};

static void init_server() {
  std::thread([] {
    asio::io_context context;
#ifdef TEST_DDS_DOMAIN
    domain_dds_server server(context, ENDPOINT);
#elif defined(TEST_DDS_SSL)
    asio::ssl::context ssl_context(asio::ssl::context::sslv23);
    {
      ssl_context.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 | asio::ssl::context::single_dh_use);
      ssl_context.set_password_callback([](std::size_t size, asio::ssl::context_base::password_purpose purpose) {
        (void)(size);
        (void)(purpose);
        return "test";
      });
      ssl_context.use_certificate_chain_file(OPENSSL_PEM_PATH "server.pem");
      ssl_context.use_private_key_file(OPENSSL_PEM_PATH "server.pem", asio::ssl::context::pem);
      ssl_context.use_tmp_dh_file(OPENSSL_PEM_PATH "dh4096.pem");
    }
    dds_server_ssl server(context, PORT, ssl_context);
#elif defined(TEST_DDS_NORMAL)
    dds_server server(context, PORT);
#endif
    server.start(true);
  }).detach();
}

static void init_client() {
  for (int i = 0; i < 3; ++i) {
    std::thread([i] {
      asio::io_context context;
#ifdef TEST_DDS_DOMAIN
      domain_dds_client client(context);
      client.open(ENDPOINT);
#elif defined(TEST_DDS_SSL)
      asio::ssl::context ssl_context(asio::ssl::context::sslv23);
      ssl_context.load_verify_file(OPENSSL_PEM_PATH "ca.pem");
      dds_client_ssl client(context, ssl_context);
      client.open("localhost", PORT);
#elif defined(TEST_DDS_NORMAL)
      dds_client client(context);
      client.open("localhost", PORT);
#endif
      // ensure open for unittest
      client.reset_reconnect(0);
      client.wait_open();

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
      client.run();
    }).detach();
  }
}

#if 1  // IDE
#ifdef TEST_DDS_DOMAIN
static void interval_check(domain_dds_client& client) {
#elif defined(TEST_DDS_SSL)
static void interval_check(dds_client_ssl& client) {
#elif defined(TEST_DDS_NORMAL)
static void interval_check(dds_client& client) {
#endif
#else
static auto interval_check = [](auto& client) {
#endif
  /// 1. test basic
  static bool first_run = true;
  static std::atomic_bool received_flag_self{false};
  static std::atomic_bool received_flag_vector{false};
  static std::atomic_bool received_flag_void{false};
  LOG("test... %d", first_run);
  if (first_run) {
    first_run = false;
    client.subscribe("topic_self", [](const std::string& msg) {
      LOG("received: topic_self: %s", msg.c_str());
      ASSERT(!received_flag_self);
      received_flag_self = true;
    });
    client.subscribe("topic_vector", [](const std::vector<uint32_t>& msg) {
      LOG("received: topic_vector: %zu", msg.size());
      ASSERT(!received_flag_vector);
      std::vector<uint32_t> data{1, 2, 3};
      ASSERT(msg == data);
      received_flag_vector = true;
    });
    client.subscribe("topic_void", [] {
      LOG("received: topic_void");
      ASSERT(!received_flag_void);
      received_flag_void = true;
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

    ASSERT(received_flag_vector);
    received_flag_vector = false;

    ASSERT(received_flag_void);
    received_flag_void = false;

    if (run_once) {
      client.stop();
    }
  }

  /// 2 common usage
  // 2.1 msg type: std::string
  client.publish<std::string>("topic_self", "to client_self");
  client.publish<std::string>("topic_0", "to client_0");
  client.publish<std::string>("topic_1", "to client_1");
  client.publish<std::string>("topic_2", "to client_2");
  client.publish<std::string>("topic_all", "hello");

  // 2.2 msg type: std::vector or any serializable type
  std::vector<uint32_t> data{1, 2, 3};
  client.publish("topic_vector", data);

  // 2.3 msg type: void
  client.publish("topic_void");

  /// 3 test unsubscribe
  // 3.1  use topic
  client.subscribe("topic_test_0", [] {
    ASSERT(false);
  });
  client.unsubscribe("topic_test_0");
  client.publish("topic_test_0");

  // 3.2  use id
  auto id = client.subscribe("topic_test_1", [] {
    ASSERT(false);
  });
  client.unsubscribe(id);
  client.publish("topic_test_1");
};

int main(int argc, char* argv[]) {
  (void)(argv);
  run_once = argc >= 2;
  // run_once = true;
  if (run_once) {
    interval_ms = 100;
  }

#ifdef TEST_DDS_DOMAIN
  ::unlink(ENDPOINT);
#endif
  init_server();
  // std::this_thread::sleep_for(std::chrono::milliseconds(10));  // wait server ready
  init_client();

  asio::io_context context;
#ifdef TEST_DDS_DOMAIN
  domain_dds_client client(context);
  client.open(ENDPOINT);
#elif defined(TEST_DDS_SSL)
  asio::ssl::context ssl_context(asio::ssl::context::sslv23);
  ssl_context.load_verify_file(OPENSSL_PEM_PATH "ca.pem");
  dds_client_ssl client(context, ssl_context);
  client.open("localhost", PORT);
#elif defined(TEST_DDS_NORMAL)
  dds_client client(context);
  client.open("localhost", PORT);
#endif
  // ensure open for unittest
  client.reset_reconnect(0);
  client.wait_open();

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
