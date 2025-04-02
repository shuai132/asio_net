#pragma once

#include "detail/dds_client_t.hpp"

namespace asio_net {

using dds_client = detail::dds_client_t<detail::socket_type::normal>;
using dds_client_ssl = detail::dds_client_t<detail::socket_type::ssl>;
using domain_dds_client = detail::dds_client_t<detail::socket_type::domain>;

}  // namespace asio_net
