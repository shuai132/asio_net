#pragma once

#include <array>
#include <functional>
#include <string>
#include <utility>

#include "asio.hpp"
#include "detail/log.h"

namespace asio_net {

/**
 * message format:
 * prefix + '\n' + name + '\n' + message
 * i.e: "discovery\nname\nmessage"
 */
namespace server_discovery {

const char* const addr_default = "239.255.0.1";
const uint16_t port_default = 30001;

class receiver {
  using service_found_handle_t = std::function<void(std::string name, std::string message)>;

 public:
  receiver(asio::io_context& io_context, service_found_handle_t handle,  // NOLINT(cppcoreguidelines-pro-type-member-init)
           std::string addr = addr_default, uint16_t port = port_default)
      : socket_(io_context), service_found_handle_(std::move(handle)), addr_(std::move(addr)), port_(port) {
    try_init();
  }

 private:
  void try_init() {
    try {
      // create the socket so that multiple may be bound to the same address.
      asio::ip::udp::endpoint listen_endpoint(asio::ip::make_address("0.0.0.0"), port_);
      socket_.open(listen_endpoint.protocol());
      socket_.set_option(asio::ip::udp::socket::reuse_address(true));
      socket_.set_option(asio::ip::multicast::join_group(asio::ip::make_address(addr_)));
      socket_.bind(listen_endpoint);
      do_receive();
    } catch (const std::exception& e) {
      ASIO_NET_LOGE("receive: init err: %s", e.what());
      auto timer = std::make_shared<asio::steady_timer>(socket_.get_executor());
      timer->expires_after(std::chrono::seconds(1));
      auto timer_p = timer.get();
      timer_p->async_wait([this, timer = std::move(timer)](const std::error_code&) mutable {
        try_init();
        timer = nullptr;
      });
    }
  }

  void do_receive() {
    socket_.async_receive_from(asio::buffer(data_), sender_endpoint_, [this](const std::error_code& ec, std::size_t length) {
      if (!ec) {
        std::vector<std::string> msgs;
        {
          msgs.reserve(3);
          auto begin = data_.data();
          auto end = data_.data() + length;
          decltype(begin) it;
          int count = 0;
          while ((it = std::find(begin, end, '\n')) != end) {
            msgs.emplace_back(begin, it);
            begin = it + 1;
            if (++count > 2) break;
          }
          msgs.emplace_back(begin, it);
        }
        if (msgs.size() == 3 && msgs[0] == "discovery") {
          service_found_handle_(std::move(msgs[1]), std::move(msgs[2]));
        }
        do_receive();
      } else {
        ASIO_NET_LOGD("server_discovery: receive: %s", ec.message().c_str());
      }
    });
  }

  asio::ip::udp::socket socket_;
  asio::ip::udp::endpoint sender_endpoint_;
  std::array<char, 1024> data_;
  service_found_handle_t service_found_handle_;
  std::string addr_;
  uint16_t port_;
};

class sender {
 public:
  sender(asio::io_context& io_context, const std::string& service_name, const std::string& message, uint32_t send_period_sec = 1,
         const char* addr = addr_default, uint16_t port = port_default)
      : endpoint_(asio::ip::make_address(addr), port),
        socket_(io_context, endpoint_.protocol()),
        timer_(io_context),
        send_period_sec_(send_period_sec),
        message_("discovery\n" + service_name + '\n' + message) {
    do_send();
  }

 private:
  void do_send() {
    socket_.async_send_to(asio::buffer(message_), endpoint_, [this](const std::error_code& ec, std::size_t /*length*/) {
      if (ec) {
        ASIO_NET_LOGE("server_discovery: sender: err: %s", ec.message().c_str());
      }
      do_send_next();
    });
  }

  void do_send_next() {
    timer_.expires_after(std::chrono::seconds(send_period_sec_));
    timer_.async_wait([this](const std::error_code& ec) {
      if (!ec) do_send();
    });
  }

 private:
  asio::ip::udp::endpoint endpoint_;
  asio::ip::udp::socket socket_;
  asio::steady_timer timer_;
  uint32_t send_period_sec_;
  std::string message_;
};

}  // namespace server_discovery
}  // namespace asio_net
