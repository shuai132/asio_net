# asio_net

[![Build Status](https://github.com/shuai132/asio_net/workflows/build/badge.svg)](https://github.com/shuai132/asio_net/actions?workflow=build)
[![Release](https://img.shields.io/github/release/shuai132/asio_net.svg)](https://github.com/shuai132/asio_net/releases)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

a tiny C++14 Async TCP/UDP/RPC/DDS library based on [asio](http://think-async.com/Asio/)
and [rpc_core](https://github.com/shuai132/rpc_core)

## Features

* Header-Only
* TCP/UDP: depend: [asio](http://think-async.com/Asio/)
* RPC: via socket/SSL, domain socket, depend: [rpc_core](https://github.com/shuai132/rpc_core)
* DDS: via socket/SSL, domain socket, depend: [rpc_core](https://github.com/shuai132/rpc_core)
* Service Discovery: based on UDP multicast
* IPv4 and IPv6
* SSL/TLS: depend OpenSSL
* Serial Port
* Automatic reconnection
* Comprehensive unittests

Options:

* TCP can be configured to automatically handle the problem of packet fragmentation to support the transmission of
  complete data packets.

* Supports setting the maximum packet length, and will automatically disconnect if exceeded.

## Requirements

* C++14
* [asio](http://think-async.com/Asio/)
* Optional: C++20 (for rpc coroutine api, co_await async_call)

## Usage

* clone

```shell
git clone --recurse-submodules git@github.com:shuai132/asio_net.git
```

or

```shell
git clone git@github.com:shuai132/asio_net.git && cd asio_net
git submodule update --init --recursive
```

The following are examples of using each module. For complete unit tests,
please refer to the source code: [test](test)

* TCP

  You can enable automatic handling of packet fragmentation using `tcp_config`.
  Subsequent send and receive will be complete data packets.

  By default, this feature is disabled.

```c++
  // echo server
  asio::io_context context;
  tcp_server server(context, PORT/*, tcp_config*/);
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
  tcp_client client(context/*, tcp_config*/);
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

  rpc based on tcp and [rpc_core](https://github.com/shuai132/rpc_core), and also support ipv6 and ssl.
  more usages, see [rpc.cpp](test/rpc.cpp) and [rpc_config.cpp](test/rpc_config.cpp)

```c++
  // server
  asio::io_context context;
  rpc_server server(context, PORT/*, rpc_config*/);
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
  rpc_client client(context/*, rpc_config*/);
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

and you can use C++20 coroutine see: [rpc_c_coroutine.cpp](test/rpc_c_coroutine.cpp)

```c++
  asio::co_spawn(context, [rpc]() -> asio::awaitable<void> {
    auto result = co_await rpc->cmd("cmd")->msg(std::string("hello"))->async_call<std::string>();
    assert(result.data == "world");
  }, asio::detached);
```

* DDS

```c++
  // run a server as daemon
  asio::io_context context;
  dds_server server(context, PORT);
  server.start(true);
```

```c++
  // client
  asio::io_context context;
  dds_client client(context);
  client.open("localhost", PORT);
  client.subscribe("topic", [](const std::string& data) {
  });
  client.publish("topic", "string/binary");
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

* Serial Port

```c++
  asio::io_context context;
  serial_port serial(context);
  serial.on_open = [&] {
    /// set_option
    serial.set_option(asio::serial_port::baud_rate(115200));
    serial.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
    serial.set_option(asio::serial_port::parity(asio::serial_port::parity::none));
    serial.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
    serial.set_option(asio::serial_port::character_size(asio::serial_port::character_size(8)));

    /// test
    serial.send("hello world");
  };
  serial.on_data = [](const std::string& data) {
  };
  serial.on_open_failed = [](std::error_code ec) {
  };
  serial.on_close = [] {
  };
  serial.open("/dev/tty.usbserial-xx");
  serial.run();
```

# Links

* RPC library for MCU

  most MCU not support asio, there is a library can be ported easily: [esp_rpc](https://github.com/shuai132/esp_rpc)
