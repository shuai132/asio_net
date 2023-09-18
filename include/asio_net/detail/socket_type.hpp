#pragma once

#include "asio.hpp"
#include "log.h"
#include "noncopyable.hpp"

#ifdef ASIO_NET_ENABLE_SSL
#include "asio/ssl.hpp"
#endif

namespace asio_net {
namespace detail {

enum class socket_type {
  normal,
  domain,
  ssl,
};

template <socket_type T>
struct socket_impl;

template <>
struct socket_impl<socket_type::normal> {
  using socket = asio::ip::tcp::socket;
  using endpoint = asio::ip::tcp::endpoint;
  using resolver = asio::ip::tcp::resolver;
  using acceptor = asio::ip::tcp::acceptor;
};

template <>
struct socket_impl<socket_type::domain> {
  using socket = asio::local::stream_protocol::socket;
  using endpoint = asio::local::stream_protocol::endpoint;
  using acceptor = asio::local::stream_protocol::acceptor;
};

#ifdef ASIO_NET_ENABLE_SSL
template <>
struct socket_impl<socket_type::ssl> {
  using socket = asio::ssl::stream<asio::ip::tcp::socket>;
  using endpoint = asio::ip::tcp::endpoint;
  using resolver = asio::ip::tcp::resolver;
  using acceptor = asio::ip::tcp::acceptor;
};
#endif

}  // namespace detail
}  // namespace asio_net
