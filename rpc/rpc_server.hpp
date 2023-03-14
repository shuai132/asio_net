#pragma once

#include "detail/rpc_server_t.hpp"

namespace asio_net {

using rpc_server = rpc_server_t<asio::ip::tcp>;
using domain_rpc_server = rpc_server_t<asio::local::stream_protocol>;

}  // namespace asio_net
