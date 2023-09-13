#pragma once

#include "detail/rpc_server_t.hpp"

namespace asio_net {

using rpc_server = detail::rpc_server_t<detail::socket_type::normal>;
using rpc_server_ssl = detail::rpc_server_t<detail::socket_type::ssl>;
using domain_rpc_server = detail::rpc_server_t<detail::socket_type::domain>;

}  // namespace asio_net
