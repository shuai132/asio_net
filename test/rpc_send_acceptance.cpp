#include <chrono>
#include <memory>
#include <string>

#include "asio_net/rpc_server.hpp"
#include "asio_net/tcp_server.hpp"
#include "assert_def.h"

struct rpc_send_fixture {
  asio::io_context context;
  asio_net::rpc_config config;
  asio_net::tcp_config channel_config;
  asio::ip::tcp::socket peer{context};
  std::shared_ptr<asio_net::tcp_session> transport;
  std::shared_ptr<asio_net::rpc_session> session;

  explicit rpc_send_fixture(uint32_t body_limit = 64, uint32_t send_limit = 128) {
    config.max_body_size = body_limit;
    config.max_send_buffer_size = send_limit;
    channel_config = config.to_tcp_config();
    asio::ip::tcp::acceptor acceptor(context, {asio::ip::tcp::v4(), 0});
    peer.connect({asio::ip::address_v4::loopback(), acceptor.local_endpoint().port()});
    transport = std::make_shared<asio_net::tcp_session>(acceptor.accept(), channel_config);
    session = std::make_shared<asio_net::rpc_session>(context, config);
    ASSERT(session->init(transport));
  }

  ~rpc_send_fixture() {
    if (transport) transport->close();
    context.restart();
    context.run_for(std::chrono::milliseconds(100));
  }
};

int main() {
  {
    rpc_send_fixture f;
    auto connection = f.session->rpc->get_connection();
    ASSERT(connection->send_package("one"));
    ASSERT(connection->send_package("two"));  // Accepted into the pending write queue.
    f.context.run_for(std::chrono::milliseconds(100));
    ASSERT(f.context.stopped());
    ASSERT(f.peer.available() == 2 * (sizeof(uint32_t) + 3));
    std::string data(f.peer.available(), '\0');
    asio::read(f.peer, asio::buffer(data));
    ASSERT(data.substr(sizeof(uint32_t), 3) == "one");
    ASSERT(data.substr(2 * sizeof(uint32_t) + 3, 3) == "two");
    ASSERT(f.session->rpc->cmd("ok")->call());
    f.context.restart();
    f.context.run_for(std::chrono::milliseconds(100));
    f.transport->close();
    ASSERT(!connection->send_package("closed"));
    f.transport.reset();
    ASSERT(!connection->send_package("expired"));
  }
  {
    rpc_send_fixture f;
    int completed = 0;
    auto request = f.session->rpc->cmd("too-large")->msg(std::string(256, 'x'))->finally([&](rpc_core::finally_t reason) {
      ASSERT(reason == rpc_core::finally_t::rpc_not_ready);
      ++completed;
    });
    auto result = request->call();
    ASSERT(!result);
    ASSERT(result.type == rpc_core::finally_t::rpc_not_ready);
    ASSERT(completed == 1);
    ASSERT(!f.transport->is_open());
  }
  {
    rpc_send_fixture f(64, 4);
    ASSERT(!f.session->rpc->get_connection()->send_package("12345"));
    ASSERT(!f.transport->is_open());
  }
}
