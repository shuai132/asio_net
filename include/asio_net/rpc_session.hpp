#pragma once

#include "asio.hpp"
#include "detail/rpc_session_t.hpp"

namespace asio_net {

using rpc_session = detail::rpc_session_t<detail::socket_type::normal>;
using rpc_session_ssl = detail::rpc_session_t<detail::socket_type::ssl>;
using domain_rpc_session = detail::rpc_session_t<detail::socket_type::domain>;

}  // namespace asio_net
