# Repository Guidelines

## Project Structure & Module Organization

`asio_net` is a header-only C++ networking library. Public headers live in `include/asio_net/`, with thin API headers such as `tcp_client.hpp` and implementation details under `include/asio_net/detail/`. The umbrella include is `include/asio_net.hpp`. The bundled `rpc_core` dependency is kept under `include/asio_net/rpc_core/` and includes its own C++ headers, tests, and Rust crate. Integration and behavior tests are in `test/`; SSL fixtures are in `test/ssl/`, and compile-only checks are in `test/compile_check/`.

## Build, Test, and Development Commands

Initialize dependencies after cloning:

```sh
git submodule update --init --recursive
git clone https://github.com/chriskohlhoff/asio.git -b asio-1-32-0 --depth=1
```

Configure and build locally:

```sh
ASIO_PATH=asio/asio/include cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Enable SSL tests when OpenSSL is available:

```sh
ASIO_PATH=asio/asio/include cmake -S . -B build -DASIO_NET_ENABLE_SSL=ON
```

Run tests directly from `build/`, for example:

```sh
./build/asio_net_test_tcp
./build/asio_net_test_udp
./build/asio_net_test_rpc
./build/asio_net_test_dds 1
```

## Coding Style & Naming Conventions

Use C++14 by default; coroutine examples and tests opt into C++20 explicitly. Follow `.clang-format`: Google style, 150-column limit, and short empty functions/lambdas may stay on one line. Use 2-space indentation. Keep public APIs in `include/asio_net/*.hpp`, implementation templates in `detail/*_t.hpp`, and test binaries named from their source module, such as `test/rpc_reconnect.cpp` producing `asio_net_test_rpc_reconnect`.

## Testing Guidelines

There is no CTest suite; add new test executables in `CMakeLists.txt` under `ASIO_NET_BUILD_TEST`. Prefer focused integration tests in `test/` named after the feature or scenario, for example `tcp_bigdata.cpp` or `rpc_c_open_close.cpp`. Keep network tests deterministic and free ports released promptly. When changing SSL, DDS, domain socket, or coroutine behavior, run the matching executable in addition to the core TCP/UDP/RPC tests.

## Commit & Pull Request Guidelines

Recent history uses short Conventional Commit prefixes such as `feat:`, `fix:`, and `refactor:`. Keep subjects imperative and specific, for example `fix: reconnect issue`. Pull requests should describe the behavior change, list the test commands run, mention platform-specific impact, and link related issues. Include logs or screenshots only when they clarify failures or CI differences.

## Security & Configuration Tips

Do not commit generated build directories or local dependency clones. Treat files in `test/ssl/` as test fixtures only. Use `ASIO_NET_DISABLE_ON_DATA_PRINT=ON` for quieter CI-style runs.
