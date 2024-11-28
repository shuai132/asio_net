#include <cstdio>

#include "asio_net/rpc_server.hpp"
#include "log.h"

using namespace asio_net;
using namespace rpc_core;

const uint16_t PORT = 6666;

int main() {
  asio::io_context context;
  rpc_server server(context, PORT);
  server.on_session = [&context](const std::weak_ptr<rpc_session>& rs) {
    auto session = rs.lock();
    LOG("on_session: %p", session.get());
    session->on_close = [rs] {
      LOG("session on_close: %p", rs.lock().get());
    };
    auto& rpc = session->rpc;

    // clang-format off
    /// scheduler for dispatch rsp to asio context
    auto scheduler_asio_dispatch = [&](auto handle) {
      asio::dispatch(context, std::move(handle));
    };
    /// scheduler for use asio coroutine
    auto scheduler_asio_coroutine = [&](auto handle) {
      asio::co_spawn(context, [handle = std::move(handle)]() -> asio::awaitable<void> {
        co_await handle();
      }, asio::detached);
    };

    /// 1. common usage
    rpc->subscribe("cmd", [&](request_response<std::string, std::string> rr) {
      // call rsp when data ready
      rr->rsp("world");
      // or run on context thread
      asio::dispatch(context, [rr = std::move(rr)]{ rr->rsp("world"); });
      // or run on context thread, use asio coroutine
      asio::co_spawn(context, [&, rr = std::move(rr)]() -> asio::awaitable<void> {
        asio::steady_timer timer(context);
        timer.expires_after(std::chrono::seconds(1));
        co_await timer.async_wait();
        rr->rsp("world");
      }, asio::detached);
    });

    /// 2. custom scheduler, automatic dispatch
    rpc->subscribe("cmd", [](const request_response<std::string, std::string>& rr) {
      rr->rsp("world");
    }, scheduler_asio_dispatch);

    /// 3. custom scheduler, simple way to use asio coroutine
    rpc->subscribe("cmd", [&](request_response<std::string, std::string> rr) -> asio::awaitable<void> {
      LOG("session on cmd: %s", rr->req.c_str());
      asio::steady_timer timer(context);
      timer.expires_after(std::chrono::seconds(1));
      co_await timer.async_wait();
      rr->rsp("world");
    }, scheduler_asio_coroutine);
    // clang-format on
  };
  server.start(true);
  return 0;
}
