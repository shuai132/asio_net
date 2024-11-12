#pragma once

#include <utility>
#include <vector>

#include "dds_type.hpp"
#include "rpc_server_t.hpp"

namespace asio_net {

template <detail::socket_type T>
class dds_server_t {
 public:
  dds_server_t(asio::io_context& io_context, uint16_t port) : server(io_context, port) {
    static_assert(T == detail::socket_type::normal, "");
    init();
  }

#ifdef ASIO_NET_ENABLE_SSL
  dds_server_t(asio::io_context& io_context, uint16_t port, asio::ssl::context& ssl_context) : server(io_context, port, ssl_context) {
    static_assert(T == detail::socket_type::ssl, "");
    init();
  }
#endif

  dds_server_t(asio::io_context& io_context, const std::string& endpoint) : server(io_context, endpoint) {
    static_assert(T == detail::socket_type::domain, "");
    init();
  }

  void start(bool loop) {
    server.start(loop);
  }

 private:
  void init() {
    server.on_session = [this](const std::weak_ptr<detail::rpc_session_t<T>>& rs) {
      ASIO_NET_LOGD("dds_server_t<%d>: on_session", (int)T);
      auto session = rs.lock();
      auto rpc = session->rpc;
      session->on_close = [this, rpc] {
        remove_rpc(rpc);
      };
      rpc->subscribe("update_topic_list", [this, rpc_wp = dds::rpc_w(rpc)](const std::vector<std::string>& topic_list) {
        update_topic_list(rpc_wp.lock(), topic_list);
      });
      rpc->subscribe("publish", [this, rpc_wp = dds::rpc_w(rpc)](const dds::Msg& msg) {
        publish(msg, rpc_wp);
      });
    };
  }

  void publish(const dds::Msg& msg, const dds::rpc_w& from_rpc) {
    auto it = topic_rpc_map.find(msg.topic);
    if (it != topic_rpc_map.cend()) {
      auto from_rpc_sp = from_rpc.lock();
      for (const auto& rpc : it->second) {
        if (rpc == from_rpc_sp) continue;
        rpc->cmd("publish")->msg(msg)->retry(-1)->call();
      }
    }
  }

  void remove_rpc(const dds::rpc_s& rpc) {
    std::vector<std::string> empty_topic;
    for (auto& kv : topic_rpc_map) {
      const auto& topic = kv.first;
      auto& rpc_set = kv.second;
      auto it = rpc_set.find(rpc);
      if (it != rpc_set.cend()) {
        rpc_set.erase(it);
        if (rpc_set.empty()) {
          empty_topic.push_back(topic);
        }
      }
    }
    for (const auto& item : empty_topic) {
      topic_rpc_map.erase(item);
    }
  }

  void update_topic_list(const dds::rpc_s& rpc, const std::vector<std::string>& topic_list) {
    for (auto& topic : topic_list) {
      topic_rpc_map[topic].insert(rpc);
    }
  }

 private:
  detail::rpc_server_t<T> server;
  std::unordered_map<std::string, std::set<dds::rpc_s>> topic_rpc_map;
};

using dds_server = dds_server_t<detail::socket_type::normal>;
using dds_server_ssl = dds_server_t<detail::socket_type::ssl>;
using domain_dds_server = dds_server_t<detail::socket_type::domain>;

}  // namespace asio_net
