#pragma once

#include "detail/dds_server_t.hpp"

namespace asio_net {

using dds_server = detail::dds_server_t<detail::socket_type::normal>;
using dds_server_ssl = detail::dds_server_t<detail::socket_type::ssl>;
using domain_dds_server = detail::dds_server_t<detail::socket_type::domain>;

}  // namespace asio_net
