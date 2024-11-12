#pragma once

#include "detail/dds_client_t.hpp"

namespace asio_net {

using dds_client = dds_client_t<detail::socket_type::normal>;
using dds_client_ssl = dds_client_t<detail::socket_type::ssl>;
using domain_dds_client = dds_client_t<detail::socket_type::domain>;

}  // namespace asio_net
