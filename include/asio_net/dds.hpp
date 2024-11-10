#pragma once

#include <utility>
#include <vector>

#include "rpc_client.hpp"
#include "rpc_server.hpp"

namespace asio_net {

namespace dds {

using rpc_s = std::shared_ptr<rpc_core::rpc>;

struct Msg {
  std::string topic;
  std::string data;
  RPC_CORE_DEFINE_TYPE_INNER(topic, data);
};

}  // namespace dds

class dds_server {
 public:
  dds_server(asio::io_context& io_context, uint16_t port) : server(io_context, port) {
    init();
  }

  void start(bool loop) {
    server.start(loop);
  }

 private:
  void init() {
    server.on_session = [this](const std::weak_ptr<rpc_session>& rs) {
      auto session = rs.lock();
      auto rpc = session->rpc;
      session->on_close = [this, rpc = session->rpc] {
        remove_rpc(rpc);
      };
      rpc->subscribe("update_topic_list", [this, rpc](const std::vector<std::string>& topic_list) {
        update_topic_list(rpc, topic_list);
      });
      rpc->subscribe("publish", [this](const dds::Msg& msg) {
        publish(msg);
      });
    };
  }

  void publish(const dds::Msg& msg) {
    auto it = topic_rpc_map.find(msg.topic);
    if (it != topic_rpc_map.cend()) {
      for (const auto& rpc : it->second) {
        rpc->cmd("publish")->msg(msg)->call();
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
  rpc_server server;
  std::unordered_map<std::string, std::set<dds::rpc_s>> topic_rpc_map;
};

class dds_client {
 public:
  explicit dds_client(asio::io_context& io_context) : client(io_context, rpc_config{.rpc = rpc}) {
    client.on_open = [&](const std::shared_ptr<rpc_core::rpc>&) {
      rpc->cmd("update_topic_list")->msg(topic_list)->retry(-1)->call();
      rpc->subscribe("publish", [this](const dds::Msg& msg) {
        dispatch_publish(msg);
      });
    };
  }

  void publish(std::string topic, std::string data) {
    auto msg = dds::Msg{.topic = std::move(topic), .data = std::move(data)};
    rpc->cmd("publish")->msg(std::move(msg))->call();
  }

  void subscribe(std::string topic, std::function<void(std::string msg)> handle) {
    auto it = topic_handle_map.find(topic);
    if (it == topic_handle_map.cend()) {
      topic_handle_map[topic] = std::move(handle);
      topic_list.push_back(std::move(topic));
      rpc->cmd("update_topic_list")->msg(topic_list)->retry(-1)->call();
    } else {
      it->second = std::move(handle);
    }
  }

  void unsubscribe(const std::string& topic) {
    auto it = topic_handle_map.find(topic);
    if (it != topic_handle_map.cend()) {
      topic_handle_map.erase(it);
      topic_list.erase(std::remove(topic_list.begin(), topic_list.end(), topic), topic_list.end());
      rpc->cmd("update_topic_list")->msg(topic_list)->retry(-1)->call();
    }
  }

  void open(std::string ip, uint16_t port) {
    client.set_reconnect(1000);
    client.open(std::move(ip), port);
  }

  void run() {
    client.run();
  }

 private:
  void dispatch_publish(const dds::Msg& msg) {
    auto it = topic_handle_map.find(msg.topic);
    if (it != topic_handle_map.cend()) {
      auto& handle = it->second;
      handle(msg.data);
    }
  }

 private:
  std::shared_ptr<rpc_core::rpc> rpc = rpc_core::rpc::create();
  rpc_client client;
  std::vector<std::string> topic_list;
  std::unordered_map<std::string, std::function<void(std::string msg)>> topic_handle_map;
};

}  // namespace asio_net
