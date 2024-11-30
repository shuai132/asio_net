#pragma once

#include <utility>
#include <vector>

#include "dds_type.hpp"
#include "rpc_client_t.hpp"

namespace asio_net {

template <detail::socket_type T>
class dds_client_t {
 public:
  explicit dds_client_t(asio::io_context& io_context) : io_context_(io_context), client_(io_context, rpc_config{.rpc = rpc_}) {
    init();
  }

#ifdef ASIO_NET_ENABLE_SSL
  explicit dds_client_t(asio::io_context& io_context, asio::ssl::context& ssl_context)
      : io_context_(io_context), client_(io_context, ssl_context, rpc_config{.rpc = rpc_}) {
    init();
  }
#endif

  template <typename D = std::string>
  void publish(std::string topic, D data = {}) {
    auto msg = dds::Msg{.topic = std::move(topic), .data = rpc_core::serialize(std::move(data))};
    dispatch_publish(msg);
    rpc_->cmd("publish")->msg(std::move(msg))->call();
  }

  template <typename F, typename std::enable_if<rpc_core::detail::callable_traits<F>::argc == 0, int>::type = 0>
  uintptr_t subscribe(const std::string& topic, F handle) {
    auto handle_sp = std::make_shared<dds::handle_t>([handle = std::move(handle)](const std::string&) mutable {
      handle();
    });
    return subscribe_raw(topic, std::move(handle_sp));
  }

  template <typename F, typename std::enable_if<rpc_core::detail::callable_traits<F>::argc == 1, int>::type = 0>
  uintptr_t subscribe(const std::string& topic, F handle) {
    auto handle_sp = std::make_shared<dds::handle_t>([handle = std::move(handle)](std::string msg) mutable {
      using F_Param = rpc_core::detail::remove_cvref_t<typename rpc_core::detail::callable_traits<F>::template argument_type<0>>;
      F_Param p;
      rpc_core::deserialize(std::move(msg), p);
      handle(std::move(p));
    });
    return subscribe_raw(topic, std::move(handle_sp));
  }

  uintptr_t subscribe_raw(const std::string& topic, dds::handle_s handle_sp) {
    auto handle_id = (uintptr_t)handle_sp.get();
    auto it = topic_handles_map_.find(topic);
    if (it == topic_handles_map_.cend()) {
      topic_handles_map_[topic].push_back(std::move(handle_sp));
      update_topic_list();
    } else {
      it->second.push_back(std::move(handle_sp));
    }
    return handle_id;
  }

  bool unsubscribe(const std::string& topic) {
    auto it = topic_handles_map_.find(topic);
    if (it != topic_handles_map_.cend()) {
      topic_handles_map_.erase(it);
      update_topic_list();
      return true;
    } else {
      return false;
    }
  }

  bool unsubscribe(uintptr_t handle_id) {
    auto it = std::find_if(topic_handles_map_.begin(), topic_handles_map_.end(), [id = handle_id](auto& p) {
      auto& vec = p.second;
      auto len_before = vec.size();
      vec.erase(std::remove_if(vec.begin(), vec.end(),
                               [id](auto& sp) {
                                 return (uintptr_t)sp.get() == id;
                               }),
                vec.end());
      auto len_after = vec.size();
      return len_before != len_after;
    });
    if (it != topic_handles_map_.end()) {
      ASIO_NET_LOGD("unsubscribe: id: %zu", handle_id);
      if (it->second.empty()) {
        topic_handles_map_.erase(it);
        update_topic_list();
      }
      return true;
    } else {
      ASIO_NET_LOGD("unsubscribe: no such id: %zu", handle_id);
      return false;
    }
  }

  void open(std::string ip, uint16_t port) {
    static_assert(T == detail::socket_type::normal || T == detail::socket_type::ssl, "");
    client_.set_reconnect(1000);
    client_.open(std::move(ip), port);
  }

  void open(std::string endpoint) {
    static_assert(T == detail::socket_type::domain, "");
    client_.set_reconnect(1000);
    client_.open(std::move(endpoint));
  }

  void close() {
    client_.close();
  }

  void run() {
    client_.run();
  }

  void stop() {
    client_.stop();
  }

  void reset_reconnect(uint32_t ms) {
    client_.set_reconnect(ms);
  }

  void wait_open() {
    while (!is_open) {
      io_context_.run_one();
    }
  }

 private:
  void init() {
    client_.on_open = [this](const dds::rpc_s&) {
      ASIO_NET_LOGD("dds_client_t<%d>: on_open", (int)T);
      rpc_->subscribe("publish", [this](const dds::Msg& msg) {
        dispatch_publish(msg);
      });
      update_topic_list();
      if (on_open) on_open();
      is_open = true;
    };
    client_.on_close = [this] {
      if (on_close) on_close();
      is_open = false;
    };
  }

  void dispatch_publish(const dds::Msg& msg) {
    auto it = topic_handles_map_.find(msg.topic);
    if (it != topic_handles_map_.cend()) {
      auto& handles = it->second;
      for (const auto& handle : handles) {
        (*handle)(msg.data);
      }
    }
  }

  void update_topic_list() {
    std::vector<std::string> topic_list;
    topic_list.reserve(topic_handles_map_.size());
    for (const auto& kv : topic_handles_map_) {
      topic_list.push_back(kv.first);
    }
    rpc_->cmd("update_topic_list")->msg(topic_list)->retry(-1)->call();
  }

 public:
  std::function<void()> on_open;
  std::function<void()> on_close;
  bool is_open = false;

 private:
  asio::io_context& io_context_;
  dds::rpc_s rpc_ = rpc_core::rpc::create();
  detail::rpc_client_t<T> client_;
  std::unordered_map<std::string, std::vector<dds::handle_s>> topic_handles_map_;
};

}  // namespace asio_net
