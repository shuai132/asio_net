# asio_net

[![Build Status](https://github.com/shuai132/asio_net/workflows/build/badge.svg)](https://github.com/shuai132/asio_net/actions?workflow=build)

a Tiny Async TCP/UDP/RPC library based on [ASIO](http://think-async.com/Asio/)
and [RpcCore](https://github.com/shuai132/RpcCore)

## Features

* 简化TCP、UDP相关程序的编写 依赖[ASIO](http://think-async.com/Asio/)
* 提供RPC实现 基于[RpcCore](https://github.com/shuai132/RpcCore)
* 局域网内服务发现 基于UDP组播

## Requirements

* C++14
* ASIO

## Usage

在自己的项目添加搜索路径

```cmake
include_directories(asio_net的目录)
```

以下是各模块的使用示例，完整的单元测试见: [test](./test)

* TCP

```c++
  // echo server
  asio::io_context context;
  tcp_server server(context, PORT);
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
  tcp_client client(context);
  client.on_open = [&] {
    client.send("hello");
  };
  client.on_data = [&](const std::string& data) {
  };
  client.on_close = [&] {
  };
  client.open("localhost", std::to_string(PORT));
  context.run();
```

* UDP

```c++
  // server
  asio::io_context context;
  udp_server server(context, PORT);
  server.on_data = [](uint8_t* data, size_t size, const udp::endpoint& from) {
  };
  context.run();
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
    session->on_close = [rs] {
    };
    session->rpc->subscribe("cmd", [](const RpcCore::String& data) -> RpcCore::String {
      return "world";
    });
  };
  server.start(true);
```

```c++
  // client
  asio::io_context context;
  rpc_client client(context);
  client.on_open = [&](const std::shared_ptr<RpcCore::Rpc>& rpc) {
    rpc->cmd("cmd")
        ->msg(RpcCore::String("hello"))
        ->rsp([&](const RpcCore::String& data) {
        })
        ->call();
  };
  client.on_close = [&] {
  };
  client.open("localhost", std::to_string(PORT));
  context.run();
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
