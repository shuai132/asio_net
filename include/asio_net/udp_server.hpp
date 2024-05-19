#pragma once

#include "asio.hpp"
#include "detail/udp_server_t.hpp"

namespace asio_net {

using udp_server = detail::udp_server_t<asio::ip::udp>;
using domain_udp_server = detail::udp_server_t<asio::local::datagram_protocol>;

}  // namespace asio_net
