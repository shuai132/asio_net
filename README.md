# asio_net

[![Build Status](https://github.com/shuai132/asio_net/workflows/build/badge.svg)](https://github.com/shuai132/asio_net/actions?workflow=build)
[![Release](https://img.shields.io/github/release/shuai132/asio_net.svg)](https://github.com/shuai132/asio_net/releases)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

a tiny C++14 Async TCP/UDP/RPC/DDS library based on [asio](http://think-async.com/Asio/)
and [rpc_core](https://github.com/shuai132/rpc_core)

## Features

* Header-Only
* TCP/UDP: support auto_pack option for tcp, will ensure packets are complete, just like websocket
* RPC: via socket(with SSL/TLS), domain socket, support c++20 coroutine for asynchronous operations
* DDS: via socket(with SSL/TLS), domain socket
* Service Discovery: based on UDP multicast
* IPv4 and IPv6
* SSL/TLS: depend OpenSSL
* Serial Port
* Automatic reconnection
* Comprehensive unittests

## Requirements

* [asio](http://think-async.com/Asio/)
* C++14
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

* RPC

  rpc based on tcp and [rpc_core](https://github.com/shuai132/rpc_core), and also support ipv6 and ssl.
  inspect the code for more details [rpc.cpp](test/rpc.cpp)

```c++
  // server
  asio::io_context context;
  rpc_server server(context, PORT/*, rpc_config*/);
  server.on_session = [](const std::weak_ptr<rpc_session>& rs) {
    auto session = rs.lock();
    session->on_close = [] {};
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
         assert(data == "world");
       })
       ->call();
  };
  client.on_close = [] {};
  client.open("localhost", PORT);
  client.run();
```

and, you can create rpc first, for more details: [rpc_config.cpp](test/rpc_config.cpp)

```c++
  // server
  auto rpc = rpc_core::rpc::create();
  rpc->subscribe("cmd", [](const std::string& data) -> std::string {
    assert(data == "hello");
    return "world";
  });
  
  asio::io_context context;
  rpc_server server(context, PORT, rpc_config{.rpc = rpc});
  server.start(true);
```

```c++
  // client
  auto rpc = rpc_core::rpc::create();
  asio::io_context context;
  rpc_client client(context, rpc_config{.rpc = rpc});
  client.open("localhost", PORT);
  client.run();

  rpc->cmd("cmd")->msg(std::string("hello"))->call();
```

and you can use C++20 coroutine:

```c++
  // server
  rpc->subscribe("cmd", [&](request_response<std::string, std::string> rr) -> asio::awaitable<void> {
    assert(rr->req == "hello");
    asio::steady_timer timer(context);
    timer.expires_after(std::chrono::seconds(1));
    co_await timer.async_wait();
    rr->rsp("world");
  }, scheduler_asio_coroutine);

  // client
  // use C++20 co_await with asio, or you can use custom async implementation, and co_await it!
  auto rsp = co_await rpc->cmd("cmd")->msg(std::string("hello"))->async_call<std::string>();
  assert(rsp.data == "world");
```

inspect the code for more
details: [rpc_s_coroutine.cpp](test/rpc_s_coroutine.cpp)
and [rpc_c_coroutine.cpp](test/rpc_c_coroutine.cpp)

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
  client.publish<std::string>("topic", "string/binary");
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
