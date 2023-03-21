#pragma once

#include <utility>

#include "asio.hpp"
#include "tcp_channel_t.hpp"
#include "type.h"

namespace asio_net {
namespace detail {

template <typename T>
class tcp_session_t : public tcp_channel_t<T>, public std::enable_shared_from_this<tcp_session_t<T>> {
  using socket = typename T::socket;

 public:
  explicit tcp_session_t(socket socket, const PackOption& pack_option, const uint32_t& max_body_size)
      : tcp_channel_t<T>(socket_, pack_option, max_body_size), socket_(std::move(socket)) {}

  void start() {
    tcp_channel_t<T>::do_read_start(tcp_session_t<T>::shared_from_this());
  }

 private:
  socket socket_;
};

template <typename T>
class tcp_server_t {
  using socket = typename T::socket;
  using endpoint = typename T::endpoint;

 public:
  tcp_server_t(asio::io_context& io_context, uint16_t port, PackOption pack_option = PackOption::DISABLE, uint32_t max_body_size = UINT32_MAX)
      : io_context_(io_context), acceptor_(io_context, endpoint(T::v4(), port)), pack_option_(pack_option), max_body_size_([&] {
          if (pack_option == PackOption::DISABLE) {
            max_body_size = std::min<uint32_t>(1024, max_body_size);
            max_body_size = std::max<uint32_t>(32, max_body_size);
          }
          return max_body_size;
        }()) {}

  tcp_server_t(asio::io_context& io_context, const std::string& endpoint, PackOption pack_option = PackOption::DISABLE,
               uint32_t max_body_size = UINT32_MAX)
      : io_context_(io_context), acceptor_(io_context, typename T::endpoint(endpoint)), pack_option_(pack_option), max_body_size_([&] {
          if (pack_option == PackOption::DISABLE) {
            max_body_size = std::min<uint32_t>(1024, max_body_size);
            max_body_size = std::max<uint32_t>(32, max_body_size);
          }
          return max_body_size;
        }()) {}

 public:
  void start(bool loop = false) {
    do_accept();
    if (loop) {
      io_context_.run();
    }
  }

 public:
  std::function<void(std::weak_ptr<tcp_session_t<T>>)> on_session;

 private:
  void do_accept() {
    acceptor_.async_accept([this](const std::error_code& ec, socket socket) {
      if (!ec) {
        auto session = std::make_shared<tcp_session_t<T>>(std::move(socket), pack_option_, max_body_size_);
        session->start();
        if (on_session) on_session(session);
        do_accept();
      } else {
        asio_net_LOGE("do_accept: %s", ec.message().c_str());
      }
    });
  }

 private:
  asio::io_context& io_context_;
  typename T::acceptor acceptor_;
  PackOption pack_option_;
  uint32_t max_body_size_;
};

}  // namespace detail
}  // namespace asio_net
