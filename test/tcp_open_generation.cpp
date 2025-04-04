#include <chrono>
#include <cstdlib>
#include <memory>

#include "asio_net/tcp_client.hpp"
#include "assert_def.h"
#include "log.h"

using namespace asio_net;

int main() {
  asio::io_context context;
  asio::ip::tcp::acceptor acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
  const auto port = acceptor.local_endpoint().port();
  auto server_socket = std::make_shared<asio::ip::tcp::socket>(context);

  bool server_accepted = false;
  acceptor.async_accept(*server_socket, [&](const std::error_code& ec) {
    if (!ec) {
      LOG("server accepted stale connection");
      server_accepted = true;
    }
  });

  tcp_client client(context);
  bool client_opened = false;
  bool client_open_failed = false;
  client.on_open = [&] {
    LOG("client on_open after close");
    client_opened = true;
    client.close();
  };
  client.on_open_failed = [&](std::error_code ec) {
    LOG("client on_open_failed after close: %d, %s", ec.value(), ec.message().c_str());
    client_open_failed = true;
  };

  client.open("127.0.0.1", port);
  client.close();

  asio::steady_timer timer(context);
  timer.expires_after(std::chrono::seconds(1));
  timer.async_wait([&](const std::error_code&) {
    context.stop();
  });
  context.run();

  ASSERT(!server_accepted);
  ASSERT(!client_opened);
  ASSERT(!client_open_failed);
  return EXIT_SUCCESS;
}
