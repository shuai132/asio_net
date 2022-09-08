#pragma once

namespace asio_net {

class noncopyable {
 protected:
  noncopyable() = default;
  ~noncopyable() = default;

 public:
  noncopyable(const noncopyable&) = delete;
  const noncopyable& operator=(const noncopyable&) = delete;
};

}  // namespace asio_net
