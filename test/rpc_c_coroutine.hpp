#pragma once

#define RPC_CORE_FEATURE_CO_CUSTOM co_custom
#define RPC_CORE_FEATURE_CO_CUSTOM_R asio::awaitable<result<R>>
#include "asio.hpp"
#include "rpc_core.hpp"

namespace rpc_core {

template <typename R, typename std::enable_if<!std::is_same<R, void>::value, int>::type>
asio::awaitable<result<R>> request::co_custom() {
  auto executor = co_await asio::this_coro::executor;
  co_return co_await asio::async_compose<decltype(asio::use_awaitable), void(result<R>)>(
      [this, &executor](auto& self) mutable {
        using ST = std::remove_reference<decltype(self)>::type;
        auto self_sp = std::make_shared<ST>(std::forward<ST>(self));
        rsp([&executor, self = std::move(self_sp)](R data, finally_t type) mutable {
          asio::dispatch(executor, [self = std::move(self), data = std::move(data), type]() {
            self->complete({type, data});
          });
        });
        call();
      },
      asio::use_awaitable);
}

template <typename R, typename std::enable_if<std::is_same<R, void>::value, int>::type>
asio::awaitable<result<R>> request::co_custom() {
  auto executor = co_await asio::this_coro::executor;
  co_return co_await asio::async_compose<decltype(asio::use_awaitable), void(result<R>)>(
      [this, &executor](auto& self) mutable {
        using ST = std::remove_reference<decltype(self)>::type;
        auto self_sp = std::make_shared<ST>(std::forward<ST>(self));
        mark_need_rsp();
        finally([&executor, self = std::move(self_sp)](finally_t type) mutable {
          asio::dispatch(executor, [self = std::move(self), type] {
            self->complete({type});
          });
        });
        call();
      },
      asio::use_awaitable);
}

}  // namespace rpc_core
