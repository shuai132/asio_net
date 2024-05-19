#pragma once

#include "asio.hpp"
#include "detail/udp_client_t.hpp"

namespace asio_net {

using udp_client = detail::udp_client_t<asio::ip::udp>;
using domain_udp_client = detail::udp_client_t<asio::local::datagram_protocol>;

}  // namespace asio_net
