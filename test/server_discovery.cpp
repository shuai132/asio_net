#include "asio_net/server_discovery.hpp"

#include <cstdio>

#include "log.h"

using namespace asio_net;

int main() {
  std::thread([] {
    asio::io_context context;
    server_discovery::receiver receiver(context, [](const std::string& name, const std::string& message) {
      LOG("receive: name: %s, message: %s", name.c_str(), message.c_str());
    });
    context.run();
  }).detach();

  std::thread([] {
    asio::io_context context;
    auto getFirstIp = [](asio::io_context& context) {
      using namespace asio::ip;
      tcp::resolver resolver(context);
      auto res = resolver.resolve(host_name(), "");
      if (!res.empty()) {
        return res.begin()->endpoint().address().to_string();
      }
      return std::string();
    };
    server_discovery::sender sender_name(context, "host_name", asio::ip::host_name());
    server_discovery::sender sender_ip(context, "ip", getFirstIp(context));
    context.run();
  }).detach();

  getchar();
  return 0;
}
