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
  explicit tcp_client_t(asio::io_context& io_context, Config config = {}) : tcp_channel_t<T>(socket_, config_), socket_(io_context), config_(config) {
    config_.init();
  }

  /**
   * connect to server
   *
   * @param host A string identifying a location. May be a descriptive name or a numeric address string.
   * @param port The port to open.
   */
  void open(const std::string& host, uint16_t port) {
    static_assert(std::is_same<asio::ip::tcp, T>::value, "");
    auto resolver = std::make_unique<typename T::resolver>(socket_.get_executor());
    auto rp = resolver.get();
    rp->async_resolve(typename T::resolver::query(host, std::to_string(port)),
                      [this, resolver = std::move(resolver)](const std::error_code& ec, const typename T::resolver::results_type& endpoints) {
                        if (!ec) {
                          asio::async_connect(socket_, endpoints, [this](const std::error_code& ec, const endpoint&) {
                            if (!ec) {
                              this->init_socket();
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

  /**
   * connect to domain socket
   *
   * @param endpoint e.g. /tmp/foobar
   */
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
  Config config_;
};

}  // namespace detail
}  // namespace asio_net
