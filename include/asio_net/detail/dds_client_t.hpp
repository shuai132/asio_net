#pragma once

#include <utility>
#include <vector>

#include "dds_type.hpp"
#include "rpc_client_t.hpp"

namespace asio_net {

template <detail::socket_type T>
class dds_client_t {
 public:
  explicit dds_client_t(asio::io_context& io_context) : client(io_context, rpc_config{.rpc = rpc}) {
    init();
  }

#ifdef ASIO_NET_ENABLE_SSL
  explicit dds_client_t(asio::io_context& io_context, asio::ssl::context& ssl_context) : client(io_context, ssl_context, rpc_config{.rpc = rpc}) {
    init();
  }
#endif

  void publish(std::string topic, std::string data = "") {
    auto msg = dds::Msg{.topic = std::move(topic), .data = std::move(data)};
    dispatch_publish(msg);
    rpc->cmd("publish")->msg(std::move(msg))->call();
  }

  uintptr_t subscribe(const std::string& topic, dds::handle_t handle) {
    auto it = topic_handles_map.find(topic);
    auto handle_sp = std::make_shared<dds::handle_t>(std::move(handle));
    auto handle_id = (uintptr_t)handle_sp.get();
    if (it == topic_handles_map.cend()) {
      topic_handles_map[topic].push_back(std::move(handle_sp));
      update_topic_list();
    } else {
      it->second.push_back(std::move(handle_sp));
    }
    return handle_id;
  }

  bool unsubscribe(const std::string& topic) {
    auto it = topic_handles_map.find(topic);
    if (it != topic_handles_map.cend()) {
      topic_handles_map.erase(it);
      update_topic_list();
      return true;
    } else {
      return false;
    }
  }

  bool unsubscribe(uintptr_t handle_id) {
    auto it = std::find_if(topic_handles_map.begin(), topic_handles_map.end(), [id = handle_id](auto& p) {
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
    if (it != topic_handles_map.end()) {
      ASIO_NET_LOGD("unsubscribe: id: %zu", handle_id);
      if (it->second.empty()) {
        topic_handles_map.erase(it);
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
    client.set_reconnect(1000);
    client.open(std::move(ip), port);
  }

  void open(std::string endpoint) {
    static_assert(T == detail::socket_type::domain, "");
    client.set_reconnect(1000);
    client.open(std::move(endpoint));
  }

  void run() {
    client.run();
  }

 private:
  void init() {
    client.on_open = [&](const std::shared_ptr<rpc_core::rpc>&) {
      ASIO_NET_LOGD("dds_client_t<%d>: on_open", (int)T);
      rpc->subscribe("publish", [this](const dds::Msg& msg) {
        dispatch_publish(msg);
      });
      update_topic_list();
    };
  }

  void dispatch_publish(const dds::Msg& msg) {
    auto it = topic_handles_map.find(msg.topic);
    if (it != topic_handles_map.cend()) {
      auto& handles = it->second;
      for (const auto& handle : handles) {
        (*handle)(msg.data);
      }
    }
  }

  void update_topic_list() {
    std::vector<std::string> topic_list;
    topic_list.reserve(topic_handles_map.size());
    for (const auto& kv : topic_handles_map) {
      topic_list.push_back(kv.first);
    }
    rpc->cmd("update_topic_list")->msg(topic_list)->retry(-1)->call();
  }

 private:
  std::shared_ptr<rpc_core::rpc> rpc = rpc_core::rpc::create();
  detail::rpc_client_t<T> client;
  std::unordered_map<std::string, std::vector<std::shared_ptr<dds::handle_t>>> topic_handles_map;
};

using dds_client = dds_client_t<detail::socket_type::normal>;
using dds_client_ssl = dds_client_t<detail::socket_type::ssl>;
using domain_dds_client = dds_client_t<detail::socket_type::domain>;

}  // namespace asio_net
