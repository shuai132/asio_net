#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "asio_net/rpc_client.hpp"
#include "asio_net/rpc_server.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main() {
  // test flags
  static std::atomic_bool pass_flag_rpc_pass{false};
  static std::atomic_bool pass_flag_session_close{false};
  static std::atomic_bool pass_flag_client_close{false};

  // server
  std::thread([] {
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
    rpc_server_ssl server(context, PORT, ssl_context);
    server.on_session = [](const std::weak_ptr<rpc_session_ssl>& rs) {
      LOG("on_session:");
      auto session = rs.lock();
      session->on_close = [] {
        LOG("session on_close:");
        pass_flag_session_close = true;
      };
      session->rpc->subscribe("cmd", [](const std::string& data) -> std::string {
        LOG("session on cmd: %s", data.c_str());
        ASSERT(data == "hello");
        return "world";
      });
    };
    server.on_handshake_error = [](std::error_code ec) {
      LOGE("on_handshake_error: %d, %s", ec.value(), ec.message().c_str());
    };
    server.start(true);
  }).detach();

  // client
  std::thread([] {
    asio::io_context context;
    asio::ssl::context ssl_context(asio::ssl::context::sslv23);
    ssl_context.load_verify_file(OPENSSL_PEM_PATH "ca.pem");
    rpc_client_ssl client(context, ssl_context);
    client.on_open = [&](const std::shared_ptr<rpc_core::rpc>& rpc) {
      LOG("client on_open:");
      rpc->cmd("cmd")
          ->msg(std::string("hello"))
          ->rsp([&](const std::string& data) {
            LOG("cmd rsp: %s", data.c_str());
            if (data == "world") {
              pass_flag_rpc_pass = true;
            }
            client.close();
          })
          ->call();
    };
    client.on_close = [&] {
      pass_flag_client_close = true;
      LOG("client on_close:");
      client.stop();
    };
    client.open("localhost", PORT);
    client.run();
  }).join();

  ASSERT(pass_flag_rpc_pass);
  ASSERT(pass_flag_client_close);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT(pass_flag_session_close);
  LOG("all rpc_session should destroyed before here!!!");

  return EXIT_SUCCESS;
}
