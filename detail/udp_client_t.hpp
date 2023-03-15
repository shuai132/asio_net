#pragma once

#include "asio.hpp"
#include "noncopyable.hpp"

namespace asio_net {
namespace detail {

template <typename T>
class udp_client_t : private noncopyable {
 public:
  using socket = typename T::socket;
  using endpoint = typename T::endpoint;

 public:
  explicit udp_client_t(asio::io_context& io_context) : socket_(io_context, typename T::endpoint(T::v4(), 0)) {
    static_assert(std::is_same<asio::ip::udp, T>::value, "");
  }

  explicit udp_client_t(asio::io_context& io_context, const std::string& endpoint) : socket_(io_context) {
    static_assert(std::is_same<asio::local::datagram_protocol, T>::value, "");
    connect(endpoint);
  }

  void send_to(std::string data, const endpoint& endpoint) {
    static_assert(std::is_same<asio::ip::udp, T>::value, "");
    auto keeper = std::make_unique<std::string>(std::move(data));
    auto k_data = keeper->data();
    auto k_size = keeper->size();
    socket_.async_send_to(asio::buffer(k_data, k_size), endpoint,
                          [keeper = std::move(keeper)](const std::error_code& /*ec*/, std::size_t /*bytes_sent*/) {});
  }

  void send_to(void* data, size_t size, const endpoint& endpoint) {
    static_assert(std::is_same<asio::ip::udp, T>::value, "");
    send_to(std::string((char*)data, size), endpoint);
  }

  void connect(const endpoint& endpoint) {
    socket_.connect(endpoint);
  }

  void send(std::string data) {
    auto keeper = std::make_unique<std::string>(std::move(data));
    auto k_data = keeper->data();
    auto k_size = keeper->size();
    socket_.async_send(asio::buffer(k_data, k_size), [keeper = std::move(keeper)](const std::error_code& /*ec*/, std::size_t /*bytes_sent*/) {});
  }

  void send(void* data, size_t size) {
    send(std::string((char*)data, size));
  }

 private:
  socket socket_;
};

}  // namespace detail
}  // namespace asio_net
