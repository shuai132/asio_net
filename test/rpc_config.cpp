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
    auto rpc = rpc_core::rpc::create();
    rpc->subscribe("cmd", [](const std::string& data) -> std::string {
      LOG("session on cmd: %s", data.c_str());
      ASSERT(data == "hello");
      return "world";
    });

    asio::io_context context;
    rpc_server server(context, PORT, rpc_config{.rpc = rpc});
    server.on_session = [&](const std::weak_ptr<rpc_session>& rs) {
      LOG("on_session:");
      auto session = rs.lock();
      ASSERT(session->rpc == rpc);
      session->on_close = [] {
        LOG("session on_close:");
        pass_flag_session_close = true;
      };
    };
    server.start(true);
  }).detach();

  // client
  std::thread([] {
    auto rpc = rpc_core::rpc::create();
    rpc->cmd("cmd")->msg(std::string("hello"))->call();  // no effect

    asio::io_context context;
    rpc_client client(context, rpc_config{.rpc = rpc});
    client.on_open = [&](const std::shared_ptr<rpc_core::rpc>& rpc_) {
      LOG("client on_open:");
      ASSERT(rpc_ == rpc);
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

    LOG("client exited");
    rpc->cmd("cmd")->msg(std::string("hello"))->call();  // no effect
  }).join();

  ASSERT(pass_flag_rpc_pass);
  ASSERT(pass_flag_client_close);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT(pass_flag_session_close);
  LOG("all rpc_session should destroyed before here!!!");

  return EXIT_SUCCESS;
}
