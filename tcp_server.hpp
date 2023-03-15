#pragma once

#include "detail/tcp_server_t.hpp"

namespace asio_net {

using tcp_session = detail::tcp_session_t<asio::ip::tcp>;
using tcp_server = detail::tcp_server_t<asio::ip::tcp>;

using domain_tcp_session = detail::tcp_session_t<asio::local::stream_protocol>;
using domain_tcp_server = detail::tcp_server_t<asio::local::stream_protocol>;

}  // namespace asio_net
