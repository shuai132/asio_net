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

static void init_server() {
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

  LOG("server running...");
  server.start(true);
}

int main(int argc, char* argv[]) {
  (void)(argc);
  (void)(argv);

#ifdef TEST_DDS_DOMAIN
  ::unlink(ENDPOINT);
#endif
  init_server();

  return 0;
}
