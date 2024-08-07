#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "asio_net/rpc_client.hpp"
#include "asio_net/rpc_server.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const char* ENDPOINT = "/tmp/foobar";

int main() {
  ::unlink(ENDPOINT);  // remove previous binding

  // test flags
  static std::atomic_bool pass_flag_rpc_pass{false};
  static std::atomic_bool pass_flag_session_close{false};
  static std::atomic_bool pass_flag_client_close{false};

  // server
  std::thread([] {
    asio::io_context context;
    domain_rpc_server server(context, ENDPOINT);
    server.on_session = [](const std::weak_ptr<domain_rpc_session>& rs) {
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
    server.start(true);
  }).detach();

  // wait domain create
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // client
  std::thread([] {
    asio::io_context context;
    domain_rpc_client client(context);
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
    client.open(ENDPOINT);
    client.run();
  }).join();

  ASSERT(pass_flag_rpc_pass);
  ASSERT(pass_flag_client_close);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT(pass_flag_session_close);
  LOG("all rpc_session should destroyed before here!!!");

  return EXIT_SUCCESS;
}
