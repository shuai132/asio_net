#include <cstdio>
#include <cstdlib>

#include "asio_net.hpp"
#include "log.h"

using namespace asio_net;

#define TEST_DDS_NORMAL

#ifdef TEST_DDS_DOMAIN
const char* ENDPOINT = "/tmp/foobar";
#elif defined(TEST_DDS_NORMAL)
const uint16_t PORT = 6666;
#elif defined(TEST_DDS_SSL)
const uint16_t PORT = 6667;
#endif

static void init_client() {
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

  LOG("subscribe:");
  client.subscribe("test_topic", [](const std::string& data) {
    LOG("client: topic:%s, data:%s", "test_topic", data.c_str());
  });

  client.publish<std::string>("test_topic", "test_data");

  LOG("client running...");
  client.run();
}

int main(int argc, char* argv[]) {
  (void)(argc);
  (void)(argv);

  init_client();

  return 0;
}
