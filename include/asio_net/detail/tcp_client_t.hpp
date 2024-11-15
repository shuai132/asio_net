#pragma once

#include "noncopyable.hpp"
#include "tcp_channel_t.hpp"

namespace asio_net {
namespace detail {

template <socket_type T>
class tcp_client_t : public tcp_channel_t<T> {
 public:
  explicit tcp_client_t(asio::io_context& io_context, tcp_config config = {})
      : tcp_channel_t<T>(socket_, config_), io_context_(io_context), socket_(io_context), config_(config) {
    config_.init();
  }

#ifdef ASIO_NET_ENABLE_SSL
  explicit tcp_client_t(asio::io_context& io_context, asio::ssl::context& ssl_context, tcp_config config = {})
      : tcp_channel_t<T>(socket_, config_), io_context_(io_context), socket_(io_context, ssl_context), config_(config) {
    config_.init();
  }
#endif

  /**
   * connect to server
   *
   * @param host A string identifying a location. May be a descriptive name or a numeric address string.
   * @param port The port to open.
   */
  void open(std::string host, uint16_t port) {
    open_ = [this, host = std::move(host), port] {
      do_open(host, port);
    };
    open_();
  }

  /**
   * connect to domain socket
   *
   * @param endpoint e.g. /tmp/foobar
   */
  void open(std::string endpoint) {
    open_ = [this, endpoint = std::move(endpoint)] {
      do_open(endpoint);
    };
    open_();
  }

  void close(bool need_reconnect = false) {
    if (!need_reconnect) {
      cancel_reconnect();
    }
    tcp_channel_t<T>::close();
  }

  void set_reconnect(uint32_t ms) {
    reconnect_ms_ = ms;
    reconnect_timer_ = std::make_unique<asio::steady_timer>(io_context_);
  }

  void cancel_reconnect() {
    if (reconnect_timer_) {
      reconnect_timer_->cancel();
      reconnect_timer_ = nullptr;
    }
  }

  void check_reconnect() {
    if (!is_open && reconnect_timer_) {
      reconnect_timer_->expires_after(std::chrono::milliseconds(reconnect_ms_));
      reconnect_timer_->async_wait([this](const asio::error_code& ec) {
        if (is_open) return;
        if (!ec) {
          open_();
        }
      });
    }
  }

  void run() {
    auto work = asio::make_work_guard(io_context_);
    io_context_.run();
  }

  void stop() {
    close();
    io_context_.stop();
  }

 private:
  void do_open(const std::string& host, uint16_t port) {
    static_assert(T == socket_type::normal || T == socket_type::ssl, "");
    auto resolver = std::make_unique<typename socket_impl<T>::resolver>(io_context_);
    auto rp = resolver.get();
    rp->async_resolve(host, std::to_string(port),
                      [this, resolver = std::move(resolver), alive = std::weak_ptr<void>(this->is_alive_)](
                          const std::error_code& ec, const typename socket_impl<T>::resolver::results_type& endpoints) mutable {
                        if (alive.expired()) return;
                        if (!ec) {
                          asio::async_connect(tcp_channel_t<T>::get_socket(), endpoints,
                                              [this, alive = std::move(alive)](const std::error_code& ec, const typename socket_impl<T>::endpoint&) {
                                                if (alive.expired()) return;
                                                async_connect_handler<T>(ec);
                                              });
                        } else {
                          if (on_open_failed) on_open_failed(ec);
                          check_reconnect();
                        }
                      });
  }

  void do_open(const std::string& endpoint) {
    static_assert(T == socket_type::domain, "");
    socket_.async_connect(typename socket_impl<T>::endpoint(endpoint), [this](const std::error_code& ec) {
      async_connect_handler<socket_type::domain>(ec);
    });
  }

  template <socket_type>
  void async_connect_handler(const std::error_code& ec);

 public:
  std::function<void()> on_open;
  std::function<void(std::error_code)> on_open_failed;
  std::function<void()> on_close;

 public:
  bool is_open = false;

 private:
  asio::io_context& io_context_;
  typename socket_impl<T>::socket socket_;
  tcp_config config_;
  std::unique_ptr<asio::steady_timer> reconnect_timer_;
  uint32_t reconnect_ms_ = 0;
  std::function<void()> open_;
};

template <>
template <>
inline void tcp_client_t<socket_type::normal>::async_connect_handler<socket_type::normal>(const std::error_code& ec) {
  if (!ec) {
    this->init_socket();
    tcp_channel_t<socket_type::normal>::on_close = [this] {
      if (tcp_client_t::on_close) tcp_client_t::on_close();
      check_reconnect();
    };
    if (on_open) on_open();
    if (reconnect_timer_) {
      reconnect_timer_->cancel();
    }
    this->do_read_start();
  } else {
    if (on_open_failed) on_open_failed(ec);
    check_reconnect();
  }
}

template <>
template <>
inline void tcp_client_t<socket_type::domain>::async_connect_handler<socket_type::domain>(const std::error_code& ec) {
  if (!ec) {
    this->init_socket();
    tcp_channel_t<socket_type::domain>::on_close = [this] {
      if (tcp_client_t::on_close) tcp_client_t::on_close();
      check_reconnect();
    };
    if (on_open) on_open();
    if (reconnect_timer_) {
      reconnect_timer_->cancel();
    }
    this->do_read_start();
  } else {
    if (on_open_failed) on_open_failed(ec);
    check_reconnect();
  }
}

#ifdef ASIO_NET_ENABLE_SSL
template <>
template <>
inline void tcp_client_t<socket_type::ssl>::async_connect_handler<socket_type::ssl>(const std::error_code& ec) {
  if (!ec) {
    this->init_socket();
    socket_.async_handshake(asio::ssl::stream_base::client, [this](const std::error_code& error) {
      if (!error) {
        tcp_channel_t<socket_type::ssl>::on_close = [this] {
          if (tcp_client_t::on_close) tcp_client_t::on_close();
          check_reconnect();
        };
        if (on_open) on_open();
        if (reconnect_timer_) {
          reconnect_timer_->cancel();
        }
        this->do_read_start();
      } else {
        if (on_open_failed) on_open_failed(error);
        check_reconnect();
      }
    });
  } else {
    if (on_open_failed) on_open_failed(ec);
    check_reconnect();
  }
}
#endif

}  // namespace detail
}  // namespace asio_net
