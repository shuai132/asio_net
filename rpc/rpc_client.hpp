#pragma once

#include "detail/rpc_client_t.hpp"

namespace asio_net {

using rpc_client = rpc_client_t<asio::ip::tcp>;
using domain_rpc_client = rpc_client_t<asio::local::stream_protocol>;

}  // namespace asio_net
