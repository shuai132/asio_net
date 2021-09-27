#include <utility>

#include "asio.hpp"
#include "tcp_channel.hpp"

namespace asio_tcp {

class tcp_session : public tcp_channel,
                    public std::enable_shared_from_this<tcp_session> {
 public:
  explicit tcp_session(tcp::socket socket, const uint32_t& max_body_size)
      : tcp_channel(socket_, max_body_size), socket_(std::move(socket)) {}

  void start() { do_read_start(shared_from_this()); }

 private:
  tcp::socket socket_;
};

class tcp_server {
 public:
  tcp_server(asio::io_context& io_context, uint16_t port,
             uint32_t max_body_size_ = 4096)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        max_body_size_(max_body_size_) {
    do_accept();
  }

 public:
  std::function<void(std::weak_ptr<tcp_session>)> onNewSession;

 private:
  void do_accept() {
    acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
      if (!ec) {
        auto session =
            std::make_shared<tcp_session>(std::move(socket), max_body_size_);
        session->start();
        if (onNewSession) onNewSession(session);
      }

      do_accept();
    });
  }

 private:
  tcp::acceptor acceptor_;
  uint32_t max_body_size_;
};

}  // namespace asio_tcp
