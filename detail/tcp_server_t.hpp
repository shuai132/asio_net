#pragma once

#include <utility>

#include "asio.hpp"
#include "tcp_channel_t.hpp"
#include "type.h"

namespace asio_net {
namespace detail {

template <typename T>
class tcp_session_t : public tcp_channel_t<T>, public std::enable_shared_from_this<tcp_session_t<T>> {
  using socket = typename T::socket;

 public:
  explicit tcp_session_t(socket socket, const Config& config) : tcp_channel_t<T>(socket_, config), socket_(std::move(socket)) {
    this->init_socket();
  }

  void start() {
    tcp_channel_t<T>::do_read_start(tcp_session_t<T>::shared_from_this());
  }

 private:
  socket socket_;
};

template <typename T>
class tcp_server_t {
  using socket = typename T::socket;
  using endpoint = typename T::endpoint;

 public:
  tcp_server_t(asio::io_context& io_context, uint16_t port, Config config = {})
      : io_context_(io_context), acceptor_(io_context, endpoint(T::v4(), port)), config_(config) {
    config_.init();
  }

  /**
   * domain socket
   *
   * @param io_context
   * @param endpoint e.g. /tmp/foobar
   * @param config
   */
  tcp_server_t(asio::io_context& io_context, const std::string& endpoint, Config config = {})
      : io_context_(io_context), acceptor_(io_context, typename T::endpoint(endpoint)), config_(config) {
    config_.init();
  }

 public:
  void start(bool loop = false) {
    do_accept();
    if (loop) {
      io_context_.run();
    }
  }

 public:
  std::function<void(std::weak_ptr<tcp_session_t<T>>)> on_session;

 private:
  void do_accept() {
    acceptor_.async_accept([this](const std::error_code& ec, socket socket) {
      if (!ec) {
        auto session = std::make_shared<tcp_session_t<T>>(std::move(socket), config_);
        session->start();
        if (on_session) on_session(session);
        do_accept();
      } else {
        asio_net_LOGE("do_accept: %s", ec.message().c_str());
      }
    });
  }

 private:
  asio::io_context& io_context_;
  typename T::acceptor acceptor_;
  Config config_;
};

}  // namespace detail
}  // namespace asio_net
