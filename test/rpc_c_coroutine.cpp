#include "rpc_c_coroutine.hpp"

#include <cstdio>

#include "asio_net/rpc_client.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

const uint16_t PORT = 6666;

int main() {
  asio::io_context context;
  rpc_client client(context);
  client.on_open = [&](const std::shared_ptr<rpc_core::rpc>& rpc) {
    LOG("client on_open:");
    asio::co_spawn(
        context,
        [rpc]() -> asio::awaitable<void> {
          // std::string
          auto rsp1 = co_await rpc->cmd("cmd")->msg(std::string("hello"))->async_call<std::string>();
          LOG("string: type: %s: data: %s", rpc_core::finally_t_str(rsp1.type), (*rsp1).c_str());
          // void
          auto rsp2 = co_await rpc->cmd("cmd")->msg(std::string("hello"))->async_call();
          LOG("void: type: %s", rpc_core::finally_t_str(rsp2.type));

          /// custom async call
          // string
          auto rsp3 = co_await rpc->cmd("cmd")->msg(std::string("hello"))->async_custom<std::string>();
          LOG("custom: string: type: %s: data: %s", rpc_core::finally_t_str(rsp3.type), rsp3.data.c_str());
          // void
          auto rsp4 = co_await rpc->cmd("cmd")->msg(std::string("hello"))->async_custom();
          LOG("custom: void: type: %s", rpc_core::finally_t_str(rsp4.type));
          // cancel
          auto rsp5 = co_await rpc->cmd("cmd")->msg(std::string("hello"))->cancel()->async_custom();
          LOG("custom: cancel: type: %s", rpc_core::finally_t_str(rsp5.type));
        },
        asio::detached);
  };
  client.on_open_failed = [](std::error_code ec) {
    LOG("client on_open_failed: %d, %s", ec.value(), ec.message().c_str());
  };
  client.on_close = [] {
    LOG("client on_close:");
  };
  client.open("localhost", PORT);
  client.run();
  return 0;
}
