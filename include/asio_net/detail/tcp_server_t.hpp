#pragma once

#include <utility>

#include "../config.hpp"
#include "asio.hpp"
#include "tcp_channel_t.hpp"

namespace asio_net {
namespace detail {

template <socket_type T>
class tcp_session_t : public tcp_channel_t<T>, public std::enable_shared_from_this<tcp_session_t<T>> {
  using socket = typename socket_impl<T>::socket;

 public:
  explicit tcp_session_t(socket socket, const tcp_config& config) : tcp_channel_t<T>(socket_, config), socket_(std::move(socket)) {
    this->init_socket();
  }

  void start() {
    tcp_channel_t<T>::do_read_start(tcp_session_t<T>::shared_from_this());
  }

#ifdef ASIO_NET_ENABLE_SSL
  void async_handshake(std::function<void(std::error_code)> handle) {
    socket_.async_handshake(asio::ssl::stream_base::server, [this, handle = std::move(handle)](const std::error_code& error) {
      handle(error);
      if (!error) {
        this->start();
      }
    });
  }
#endif

 private:
  socket socket_;
};

template <socket_type T>
class tcp_server_t {
  using socket = typename socket_impl<T>::socket;
  using endpoint = typename socket_impl<T>::endpoint;

 public:
  tcp_server_t(asio::io_context& io_context, uint16_t port, tcp_config config = {})
      : io_context_(io_context),
        acceptor_(io_context, endpoint(config.enable_ipv6 ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), port)),
        config_(config) {
    config_.init();
  }

#ifdef ASIO_NET_ENABLE_SSL
  tcp_server_t(asio::io_context& io_context, uint16_t port, asio::ssl::context& ssl_context, tcp_config config = {})
      : io_context_(io_context), ssl_context_(ssl_context), acceptor_(io_context, endpoint(asio::ip::tcp::v4(), port)), config_(config) {
    config_.init();
  }
#endif

  /**
   * domain socket
   *
   * @param io_context
   * @param endpoint e.g. /tmp/foobar
   * @param config
   */
  tcp_server_t(asio::io_context& io_context, const std::string& endpoint, tcp_config config = {})
      : io_context_(io_context), acceptor_(io_context, typename socket_impl<T>::endpoint(endpoint)), config_(config) {
    config_.init();
  }

 public:
  void start(bool loop = false) {
    do_accept<T>();
    if (loop) {
      io_context_.run();
    }
  }

 public:
  std::function<void(std::weak_ptr<tcp_session_t<T>>)> on_session;

#ifdef ASIO_NET_ENABLE_SSL
  std::function<void(std::error_code)> on_handshake_error;
#endif

 private:
  template <socket_type>
  void do_accept();

 private:
  asio::io_context& io_context_;
#ifdef ASIO_NET_ENABLE_SSL
  typename std::conditional<T == socket_type::ssl, asio::ssl::context&, uint8_t>::type ssl_context_;
#endif
  typename socket_impl<T>::acceptor acceptor_;
  tcp_config config_;
};

template <>
template <>
inline void tcp_server_t<socket_type::normal>::do_accept<socket_type::normal>() {
  acceptor_.async_accept([this](const std::error_code& ec, socket socket) {
    if (!ec) {
      auto session = std::make_shared<tcp_session_t<socket_type::normal>>(std::move(socket), config_);
      session->start();
      if (on_session) on_session(session);
      tcp_server_t<socket_type::normal>::do_accept<socket_type::normal>();
    } else {
      ASIO_NET_LOGD("do_accept: %s", ec.message().c_str());
    }
  });
}

template <>
template <>
inline void tcp_server_t<socket_type::domain>::do_accept<socket_type::domain>() {
  acceptor_.async_accept([this](const std::error_code& ec, socket socket) {
    if (!ec) {
      auto session = std::make_shared<tcp_session_t<socket_type::domain>>(std::move(socket), config_);
      session->start();
      if (on_session) on_session(session);
      tcp_server_t<socket_type::domain>::do_accept<socket_type::domain>();
    } else {
      ASIO_NET_LOGD("do_accept: %s", ec.message().c_str());
    }
  });
}

#ifdef ASIO_NET_ENABLE_SSL
template <>
template <>
inline void tcp_server_t<socket_type::ssl>::do_accept<socket_type::ssl>() {
  acceptor_.async_accept([this](const std::error_code& ec, asio::ip::tcp::socket socket) {
    if (!ec) {
      using ssl_stream = typename socket_impl<socket_type::ssl>::socket;
      auto session = std::make_shared<tcp_session_t<socket_type::ssl>>(ssl_stream(std::move(socket), ssl_context_), config_);
      session->async_handshake([this, session](const std::error_code& error) {
        if (!error) {
          if (on_session) on_session(session);
        } else {
          if (on_handshake_error) on_handshake_error(error);
        }
      });
      do_accept<socket_type::ssl>();
    } else {
      ASIO_NET_LOGD("do_accept: %s", ec.message().c_str());
    }
  });
}
#endif

}  // namespace detail
}  // namespace asio_net
