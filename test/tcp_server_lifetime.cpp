#include <chrono>
#include <cstdint>
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
    bool connected = false;
    peer.async_connect(endpoint, [&](const std::error_code& ec) {
      ASSERT(!ec);
      connected = true;
    });
    context.restart();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ((!connected || calls < expected) && std::chrono::steady_clock::now() < deadline) {
      if (context.run_one_until(deadline) == 0) break;
    }
    ASSERT(calls == expected);
    ASSERT(connected);
  }
  ASSERT(!server);
  // Drain cancelled operations too, so callbacks queued before destruction run.
  context.restart();
  context.run_for(std::chrono::seconds(5));
  ASSERT(context.stopped());
}

// Destroy a client with a raw, frame-header, or frame-body read still pending.
// MSVC Debug checks that Asio's receive buffer survives cancellation completion.
void check_destroy_client(bool auto_pack, bool partial_frame) {
  asio::io_context context;
  asio::ip::tcp::acceptor acceptor(context, {asio::ip::tcp::v4(), 0});
  asio::ip::tcp::socket peer(context);
  bool accepted = false;
  acceptor.async_accept(peer, [&](const std::error_code& ec) {
    ASSERT(!ec);
    accepted = true;
  });
  asio_net::tcp_config config;
  config.auto_pack = auto_pack;
  auto client = std::make_unique<asio_net::tcp_client>(context, config);
  bool opened = false;
  client->on_open = [&] { opened = true; };
  client->on_open_failed = [](const std::error_code& ec) { ASSERT(!ec); };
  client->open("127.0.0.1", acceptor.local_endpoint().port());
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while ((!opened || !accepted) && std::chrono::steady_clock::now() < deadline) {
    if (context.run_one_until(deadline) == 0) break;
  }
  ASSERT(opened && accepted);
  if (partial_frame) {
    const uint32_t length = 1024;
    asio::write(peer, asio::buffer(&length, sizeof(length)));
    // Process the header and leave the body read pending.
    ASSERT(context.run_one_until(deadline) != 0);
  }
  client.reset();
  peer.close();
  context.restart();
  context.run_for(std::chrono::seconds(5));
  ASSERT(context.stopped());
}

int main() {
  check_destroy_client(false, false);
  check_destroy_client(true, false);
  check_destroy_client(true, true);
  {
    asio::io_context context;
    auto client = std::make_unique<asio_net::tcp_client>(context);
    client->set_reconnect(10000);
    client->check_reconnect();
    client.reset();
    context.run_for(std::chrono::seconds(5));
    ASSERT(context.stopped());
  }
  for (bool auto_pack : {false, true}) {
    asio::io_context context;
    asio::ip::tcp::acceptor probe(context, {asio::ip::tcp::v4(), 0});
    auto port = probe.local_endpoint().port();
    probe.close();
    asio_net::tcp_config config;
    config.auto_pack = auto_pack;
    auto server = std::make_unique<asio_net::tcp_server>(context, port, config);
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
