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

  using result_cb = std::function<void(const std::error_code&, std::size_t)>;

 public:
  explicit udp_client_t(asio::io_context& io_context) : socket_(create_socket<T>(io_context)) {}

  void send_to(std::string data, const endpoint& endpoint, result_cb cb = nullptr) {
    auto keeper = std::make_unique<std::string>(std::move(data));
    auto k_data = keeper->data();
    auto k_size = keeper->size();
    socket_.async_send_to(asio::buffer(k_data, k_size), endpoint,
                          [keeper = std::move(keeper), cb = std::move(cb)](const std::error_code& ec, std::size_t size) {
                            if (cb) cb(ec, size);
                          });
  }

  void send_to(void* data, size_t size, const endpoint& endpoint, result_cb cb = nullptr) {
    send_to(std::string((char*)data, size), endpoint, std::move(cb));
  }

  /**
   * connect to server, for `@send` api
   *
   * @param endpoint
   */
  void connect(const endpoint& endpoint) {
    socket_.connect(endpoint);
  }

  void send(std::string data, result_cb cb = nullptr) {
    auto keeper = std::make_unique<std::string>(std::move(data));
    auto k_data = keeper->data();
    auto k_size = keeper->size();
    socket_.async_send(asio::buffer(k_data, k_size), [keeper = std::move(keeper), cb = std::move(cb)](const std::error_code& ec, std::size_t size) {
      if (cb) cb(ec, size);
    });
  }

  void send(void* data, size_t size, result_cb cb = nullptr) {
    send(std::string((char*)data, size), std::move(cb));
  }

 private:
  template <typename Type>
  static typename std::enable_if<std::is_same<Type, asio::ip::udp>::value, typename Type::socket>::type create_socket(asio::io_context& io_context) {
    return typename Type::socket(io_context, typename Type::endpoint());
  }

  template <typename Type>
  static typename std::enable_if<std::is_same<Type, asio::local::datagram_protocol>::value, typename Type::socket>::type create_socket(
      asio::io_context& io_context) {
    typename Type::socket s(io_context);
    s.open();
    return s;
  }

 private:
  socket socket_;
};

}  // namespace detail
}  // namespace asio_net
