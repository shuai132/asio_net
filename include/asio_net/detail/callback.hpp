#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace asio_net {
namespace detail {

// Retain the callable while it runs, even when it destroys its owning object.
// Sharing the original target also preserves mutable lambda state between calls.
template <typename Signature>
struct shared_callback;

template <typename R, typename... Args>
struct shared_callback<R(Args...)> {
  std::shared_ptr<std::function<R(Args...)>> target;

  R operator()(Args... args) const {
    return (*target)(std::forward<Args>(args)...);
  }
};

template <typename R, typename... Args>
std::function<R(Args...)> hold_callback(std::function<R(Args...)>& callback) {
  using holder = shared_callback<R(Args...)>;
  if (callback && !callback.template target<holder>()) {
    callback = holder{std::make_shared<std::function<R(Args...)>>(std::move(callback))};
  }
  return callback;
}

}  // namespace detail
}  // namespace asio_net
