#pragma once

#include "detail/rpc_session_t.hpp"

namespace asio_net {

using rpc_session = rpc_session_t<asio::ip::tcp>;
using domain_rpc_session = rpc_session_t<asio::local::stream_protocol>;

}  // namespace asio_net
