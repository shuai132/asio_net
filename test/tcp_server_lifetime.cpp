#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

#include "asio_net/tcp_client.hpp"
#include "asio_net/tcp_server.hpp"
#include "assert_def.h"

template <typename Server, typename Protocol>
void check_destroy_in_callback(asio::io_context& context, std::unique_ptr<Server> server,
                               const typename Protocol::endpoint& endpoint) {
  int calls = 0;
  server->on_session = [&, seen = 0](auto weak) mutable {
    auto session = weak.lock();
    ASSERT(session);
    session->close();
    if (++seen == 2) server.reset();
    calls = seen;  // The callable must survive destroying its owner.
  };
  server->start();
  for (int expected = 1; expected <= 2; ++expected) {
    typename Protocol::socket peer(context);
    peer.connect(endpoint);
    context.restart();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (calls < expected && std::chrono::steady_clock::now() < deadline) {
      if (context.run_one_until(deadline) == 0) break;
    }
    ASSERT(calls == expected);
  }
  ASSERT(!server);
  // Drain cancelled operations too, so callbacks queued before destruction run.
  context.restart();
  context.run_for(std::chrono::seconds(5));
  ASSERT(context.stopped());
}

int main() {
  {
    asio::io_context context;
    auto client = std::make_unique<asio_net::tcp_client>(context);
    client->set_reconnect(10000);
    client->check_reconnect();
    client.reset();
    context.run_for(std::chrono::seconds(5));
    ASSERT(context.stopped());
  }
  {
    asio::io_context context;
    asio::ip::tcp::acceptor probe(context, {asio::ip::tcp::v4(), 0});
    auto port = probe.local_endpoint().port();
    probe.close();
    auto server = std::make_unique<asio_net::tcp_server>(context, port);
    check_destroy_in_callback<asio_net::tcp_server, asio::ip::tcp>(
        context, std::move(server), {asio::ip::address_v4::loopback(), port});
  }
// Match the domain socket tests in CI: Asio also defines this feature macro on
// Windows, where it does not guarantee that binding a Unix socket is supported.
#if defined(ASIO_HAS_LOCAL_SOCKETS) && !defined(_WIN32)
  {
    asio::io_context context;
    auto endpoint = std::string("asio-net-lifetime-") +
                    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sock";
    auto server = std::make_unique<asio_net::domain_tcp_server>(context, endpoint);
    check_destroy_in_callback<asio_net::domain_tcp_server, asio::local::stream_protocol>(context, std::move(server),
                                                                                       asio::local::stream_protocol::endpoint(endpoint));
    ASSERT(std::remove(endpoint.c_str()) == 0);
  }
#endif
}
