#pragma once

#include "asio.hpp"
#include "detail/tcp_client_t.hpp"

namespace asio_net {

using tcp_client = detail::tcp_client_t<detail::socket_type::normal>;
using tcp_client_ssl = detail::tcp_client_t<detail::socket_type::ssl>;
using domain_tcp_client = detail::tcp_client_t<detail::socket_type::domain>;

}  // namespace asio_net
