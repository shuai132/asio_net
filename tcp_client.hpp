#pragma once

#include "detail/tcp_client_t.hpp"

namespace asio_net {

using tcp_client = detail::tcp_client_t<asio::ip::tcp>;
using domain_tcp_client = detail::tcp_client_t<asio::local::stream_protocol>;

}  // namespace asio_net
