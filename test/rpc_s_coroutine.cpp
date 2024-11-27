#include <cstdio>

#include "asio_net/rpc_server.hpp"
#include "log.h"

using namespace asio_net;

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
    session->rpc->subscribe("cmd", [&](rpc_core::request_response<std::string, std::string> rr) {
      asio::co_spawn(
          context,
          [&context, rr = std::move(rr)]() -> asio::awaitable<void> {
            LOG("session on cmd: %s", rr->req.c_str());
            asio::steady_timer timer(context);
            timer.expires_after(std::chrono::seconds(1));
            co_await timer.async_wait();
            rr->rsp("world");
          },
          asio::detached);
    });
  };
  server.start(true);
  return 0;
}
