#pragma once

#include "noncopyable.hpp"
#include "tcp_channel_t.hpp"

namespace asio_net {
namespace detail {

template <typename T>
class tcp_client_t : public tcp_channel_t<T> {
  using socket = typename T::socket;
  using endpoint = typename T::endpoint;

 public:
  explicit tcp_client_t(asio::io_context& io_context, PackOption pack_option = PackOption::DISABLE, uint32_t max_body_size = UINT32_MAX)
      : tcp_channel_t<T>(socket_, pack_option_, max_body_size), socket_(io_context), pack_option_(pack_option) {}

  void open(const std::string& ip, uint16_t port) {
    static_assert(std::is_same<asio::ip::tcp, T>::value, "");
    auto resolver = std::make_unique<typename T::resolver>(socket_.get_executor());
    auto rp = resolver.get();
    rp->async_resolve(typename T::resolver::query(ip, std::to_string(port)),
                      [this, resolver = std::move(resolver)](const std::error_code& ec, const typename T::resolver::results_type& endpoints) {
                        if (!ec) {
                          asio::async_connect(socket_, endpoints, [this](const std::error_code& ec, const endpoint&) {
                            if (!ec) {
                              if (on_open) on_open();
                              this->do_read_start();
                            } else {
                              if (on_open_failed) on_open_failed(ec);
                            }
                          });
                        } else {
                          if (on_open_failed) on_open_failed(ec);
                        }
                      });
  }

  void open(const std::string& endpoint) {
    static_assert(std::is_same<T, asio::local::stream_protocol>::value, "");
    socket_.async_connect(typename T::endpoint(endpoint), [this](const std::error_code& ec) {
      if (!ec) {
        if (on_open) on_open();
        this->do_read_start();
      } else {
        if (on_open_failed) on_open_failed(ec);
      }
    });
  }

 public:
  std::function<void()> on_open;
  std::function<void(std::error_code)> on_open_failed;

 private:
  socket socket_;
  PackOption pack_option_;
};

}  // namespace detail
}  // namespace asio_net
