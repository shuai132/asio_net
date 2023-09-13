#include "assert_def.h"
#include "log.h"
#include "tcp_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  asio::io_context context;
  asio::ssl::context ssl_context(asio::ssl::context::sslv23);
  {
    ssl_context.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 | asio::ssl::context::single_dh_use);
    ssl_context.set_password_callback(std::bind([] {  // NOLINT
      return "test";
    }));
    ssl_context.use_certificate_chain_file(OPENSSL_PEM_PATH "server.pem");
    ssl_context.use_private_key_file(OPENSSL_PEM_PATH "server.pem", asio::ssl::context::pem);
    ssl_context.use_tmp_dh_file(OPENSSL_PEM_PATH "dh4096.pem");
  }
  tcp_server_ssl server(context, PORT, ssl_context);
  server.on_session = [](const std::weak_ptr<tcp_session_ssl>& ws) {
    LOG("on_session:");
    auto session = ws.lock();
    session->on_close = [] {
      LOG("session on_close:");
    };
    session->on_data = [ws](std::string data) {
      ASSERT(!ws.expired());
      LOG("session on_data: %zu, %s", data.size(), data.c_str());
      ws.lock()->send(std::move(data));
    };
  };
  server.on_handshake_error = [](std::error_code ec) {
    LOGE("on_handshake_error: %d, %s", ec.value(), ec.message().c_str());
  };
  server.start(true);
  return 0;
}
