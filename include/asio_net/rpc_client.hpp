#pragma once

#include "asio.hpp"
#include "detail/rpc_client_t.hpp"

namespace asio_net {

using rpc_client = detail::rpc_client_t<detail::socket_type::normal>;
using rpc_client_ssl = detail::rpc_client_t<detail::socket_type::ssl>;
using domain_rpc_client = detail::rpc_client_t<detail::socket_type::domain>;

}  // namespace asio_net
