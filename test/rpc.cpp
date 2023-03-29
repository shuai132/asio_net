#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "assert_def.h"
#include "log.h"
#include "rpc_client.hpp"
#include "rpc_server.hpp"

using namespace asio_net;

const uint16_t PORT = 6666;

int main(int argc, char** argv) {
  // test flags
  static std::atomic_bool pass_flag_rpc_pass{false};
  static std::atomic_bool pass_flag_session_close{false};
  static std::atomic_bool pass_flag_client_close{false};

  // server
  std::thread([] {
    asio::io_context context;
    static rpc_server server(context, PORT);  // static for test session lifecycle
    server.on_session = [](const std::weak_ptr<rpc_session>& rs) {
      LOG("on_session:");
      auto session = rs.lock();
      session->on_close = [] {
        LOG("session on_close:");
        pass_flag_session_close = true;
      };
      session->rpc->subscribe("cmd", [](const RpcCore::String& data) -> RpcCore::String {
        LOG("session on cmd: %s", data.c_str());
        ASSERT(data == "hello");
        return "world";
      });
    };
    server.start(true);
  }).detach();

  // client
  std::thread([] {
    asio::io_context context;
    static rpc_client client(context);  // static for test session lifecycle
    client.on_open = [&](const std::shared_ptr<RpcCore::Rpc>& rpc) {
      LOG("client on_open:");
      rpc->cmd("cmd")
          ->msg(RpcCore::String("hello"))
          ->rsp([&](const RpcCore::String& data) {
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
      context.stop();
    };
    client.open("localhost", PORT);
    context.run();
  }).join();

  ASSERT(pass_flag_rpc_pass);
  ASSERT(pass_flag_client_close);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT(pass_flag_session_close);
  LOG("all rpc_session should destroyed before here!!!");

  return EXIT_SUCCESS;
}
