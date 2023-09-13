# asio_net

[![Build Status](https://github.com/shuai132/asio_net/workflows/build/badge.svg)](https://github.com/shuai132/asio_net/actions?workflow=build)

a tiny Async TCP/UDP/RPC library based on [asio](http://think-async.com/Asio/)
and [rpc_core](https://github.com/shuai132/rpc_core)

## Features

* Header-Only
* TCP/UDP support, rely on: [asio](http://think-async.com/Asio/)
* RPC support, rely on: [rpc_core](https://github.com/shuai132/rpc_core)
* Service discovery based on UDP multicast
* Support IPv6 and SSL (with OpenSSL)
* Domain socket and rpc support
* Comprehensive unittests
* Automatic reconnection

Options:

* TCP can be configured to automatically handle the problem of packet fragmentation to support the transmission of
  complete data packets.

* Supports setting the maximum packet length, and will automatically disconnect if exceeded.

## Requirements

* C++14
* asio

## Usage

The following are examples of using each module. For complete unit tests,
please refer to the source code: [test](./test)

* TCP

You can enable automatic handling of packet fragmentation using `config`.
Subsequent send and receive will be complete data packets.

By default, this feature is disabled.

```c++
  // echo server
  asio::io_context context;
  tcp_server server(context, PORT/*, config*/);
  server.on_session = [](const std::weak_ptr<tcp_session>& ws) {
    auto session = ws.lock();
    session->on_close = [] {
    };
    session->on_data = [ws](std::string data) {
      ws.lock()->send(std::move(data));
    };
  };
  server.start(true);
```

```c++
  // echo client
  asio::io_context context;
  tcp_client client(context/*, config*/);

  client.on_data = [](const std::string& data) {
  };
  client.on_close = [] {
  };
  client.open("localhost", PORT);
  client.run();
```

* UDP

```c++
  // server
  asio::io_context context;
  udp_server server(context, PORT);
  server.on_data = [](uint8_t* data, size_t size, const udp::endpoint& from) {
  };
  server.start();
```

```c++
  // client
  asio::io_context context;
  udp_client client(context);
  auto endpoint = udp::endpoint(asio::ip::address_v4::from_string("127.0.0.1"), PORT);
  client.send_to("hello", endpoint);
  context.run();
```

* RPC

```c++
  // server
  asio::io_context context;
  rpc_server server(context, PORT);
  server.on_session = [](const std::weak_ptr<rpc_session>& rs) {
    auto session = rs.lock();
    session->on_close = [] {
    };
    session->rpc->subscribe("cmd", [](const std::string& data) -> std::string {
      return "world";
    });
  };
  server.start(true);
```

```c++
  // client
  asio::io_context context;
  rpc_client client(context);
  client.on_open = [](const std::shared_ptr<rpc_core::rpc>& rpc) {
    rpc->cmd("cmd")
        ->msg(std::string("hello"))
        ->rsp([](const std::string& data) {
        })
        ->call();
  };
  client.on_close = [] {
  };
  client.open("localhost", PORT);
  client.run();
```

* Server Discovery

```c++
  // receiver
  asio::io_context context;
  server_discovery::receiver receiver(context, [](const std::string& name, const std::string& message) {
    printf("receive: name: %s, message: %s\n", name.c_str(), message.c_str());
  });
  context.run();
```

```c++
  // sender
  asio::io_context context;
  server_discovery::sender sender_ip(context, "ip", "message");
  context.run();
```

# Links

* RPC library for MCU

  most MCU not support asio, there is a library can be ported easily: [esp_rpc](https://github.com/shuai132/esp_rpc)
