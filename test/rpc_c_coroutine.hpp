#pragma once

#define RPC_CORE_FEATURE_ASYNC_CUSTOM async_custom
#include "asio.hpp"
#include "rpc_core.hpp"

namespace rpc_core {

template <typename R, typename std::enable_if<!std::is_same<R, void>::value, int>::type>
auto request::async_custom() {
  asio::use_awaitable_t<> use_awaitable = {};
  return asio::async_initiate<asio::use_awaitable_t<>, void(rpc_core::request::future_ret<R>)>(
      [this]<typename Handler>(Handler&& h) mutable {
        auto handler = std::make_shared<Handler>(std::forward<Handler>(h));
        rsp([handler](R data) mutable {
          rpc_core::request::future_ret<R> ret;
          ret.type = finally_t::normal;
          ret.data = std::move(data);
          (*handler)(std::move(ret));
        });
        finally([handler = std::move(handler)](finally_t type) {
          if (type != finally_t::normal) {
            rpc_core::request::future_ret<R> ret;
            ret.type = type;
            (*handler)(std::move(ret));
          }
        });
        call();
      },
      use_awaitable);
}

template <typename R, typename std::enable_if<std::is_same<R, void>::value, int>::type>
auto request::async_custom() {
  asio::use_awaitable_t<> use_awaitable = {};
  return asio::async_initiate<asio::use_awaitable_t<>, void(rpc_core::request::future_ret<void>)>(
      [this]<typename Handler>(Handler&& h) mutable {
        auto handler = std::make_shared<Handler>(std::forward<Handler>(h));
        rsp([handler]() mutable {
          rpc_core::request::future_ret<R> ret;
          ret.type = finally_t::normal;
          (*handler)(std::move(ret));
        });
        finally([handler = std::move(handler)](finally_t type) {
          if (type != finally_t::normal) {
            rpc_core::request::future_ret<R> ret;
            ret.type = type;
            (*handler)(std::move(ret));
          }
        });
        call();
      },
      use_awaitable);
}

}  // namespace rpc_core
