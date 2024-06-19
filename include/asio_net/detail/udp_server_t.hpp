#pragma once

#include "asio.hpp"
#include "log.h"
#include "noncopyable.hpp"

namespace asio_net {
namespace detail {

template <typename T>
class udp_server_t : private noncopyable {
 public:
  using socket = typename T::socket;
  using endpoint = typename T::endpoint;

 public:
  udp_server_t(asio::io_context& io_context, short port, uint16_t max_length = 4096)
      : io_context_(io_context), socket_(io_context, typename T::endpoint(T::v4(), port)), max_length_(max_length) {
    data_.resize(max_length);
    do_receive();
  }

  /**
   * domain socket
   *
   * @param io_context
   * @param endpoint e.g. /tmp/foobar
   * @param max_length
   */
  udp_server_t(asio::io_context& io_context, const std::string& endpoint, uint16_t max_length = 4096)
      : io_context_(io_context), socket_(io_context, typename T::endpoint(endpoint)), max_length_(max_length) {
    data_.resize(max_length);
    do_receive();
  }

  void start() {
    io_context_.run();
  }

  std::function<void(uint8_t* data, size_t size, endpoint from)> on_data;

 private:
  void do_receive() {
    socket_.async_receive_from(asio::buffer((void*)data_.data(), max_length_), from_endpoint_, [this](const std::error_code& ec, size_t length) {
      if (!ec && length > 0) {
        if (on_data) on_data((uint8_t*)data_.data(), length, from_endpoint_);
        do_receive();
      } else {
        ASIO_NET_LOGD("udp_server error: %s", ec.message().c_str());
      }
    });
  }

 private:
  asio::io_context& io_context_;
  socket socket_;
  endpoint from_endpoint_;
  uint16_t max_length_;
  std::string data_;
};

}  // namespace detail
}  // namespace asio_net
