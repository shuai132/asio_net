#pragma once

#include "asio.hpp"
#include "detail/tcp_server_t.hpp"

namespace asio_net {

using tcp_session = detail::tcp_session_t<detail::socket_type::normal>;
using tcp_session_ssl = detail::tcp_session_t<detail::socket_type::ssl>;
using tcp_server = detail::tcp_server_t<detail::socket_type::normal>;
using tcp_server_ssl = detail::tcp_server_t<detail::socket_type::ssl>;

using domain_tcp_session = detail::tcp_session_t<detail::socket_type::domain>;
using domain_tcp_server = detail::tcp_server_t<detail::socket_type::domain>;

}  // namespace asio_net
