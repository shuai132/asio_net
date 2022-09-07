#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "assert_def.h"
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
    rpc_server server(context, PORT);
    server.on_session = [](const std::weak_ptr<rpc_session>& rs) {
      printf("on_session:\n");
      auto session = rs.lock();
      session->on_close = [] {
        printf("session on_close:\n");
        pass_flag_session_close = true;
      };
      session->rpc->subscribe<RpcCore::String, RpcCore::String>("cmd", [](const RpcCore::String& data) {
        printf("session on cmd: %s\n", data.c_str());
        ASSERT(data == "hello");
        return "world";
      });
    };
    server.start(true);
  }).detach();

  // client
  std::thread([] {
    asio::io_context context;
    rpc_client client(context);
    client.on_open = [&](const std::shared_ptr<RpcCore::Rpc>& rpc) {
      printf("client on_open:\n");
      rpc->createRequest()
          ->cmd("cmd")
          ->msg(RpcCore::String("hello"))
          ->rsp<RpcCore::String>([&](const RpcCore::String& data) {
            printf("cmd rsp: %s\n", data.c_str());
            if (data == "world") {
              pass_flag_rpc_pass = true;
            }
            client.close();
          })
          ->call();
    };
    client.on_close = [&] {
      pass_flag_client_close = true;
      printf("client on_close:\n");
      context.stop();
    };
    client.open("localhost", std::to_string(PORT));
    context.run();
  }).join();

  ASSERT(pass_flag_rpc_pass);
  ASSERT(pass_flag_client_close);
  ASSERT(pass_flag_session_close);

  return EXIT_SUCCESS;
}
