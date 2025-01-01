#pragma once

#define ASIO_NET_VER_MAJOR 2
#define ASIO_NET_VER_MINOR 1
#define ASIO_NET_VER_PATCH 0

#define ASIO_NET_TO_VERSION(major, minor, patch) (major * 10000 + minor * 100 + patch)
#define ASIO_NET_VERSION ASIO_NET_TO_VERSION(ASIO_NET_VER_MAJOR, ASIO_NET_VER_MINOR, ASIO_NET_VER_PATCH)
