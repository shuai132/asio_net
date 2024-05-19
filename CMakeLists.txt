cmake_minimum_required(VERSION 3.5)

project(asio_net CXX)

option(ASIO_NET_ENABLE_SSL "" OFF)
option(ASIO_NET_BUILD_TEST "" OFF)
option(ASIO_NET_DISABLE_ON_DATA_PRINT "" OFF)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(ASIO_NET_BUILD_TEST ON)
endif ()

if (MSVC)
    set(CMAKE_CXX_STANDARD 20)
    add_compile_options(/Zc:preprocessor)
    add_compile_options(/utf-8)
    add_compile_options(-DNOMINMAX)
    add_compile_options(-D_WIN32_WINNT=0x0601)
else ()
    set(CMAKE_CXX_STANDARD 14)
    add_compile_options(-Wall -Wunused-parameter)
endif ()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
        include
        include/asio_net/rpc_core/include
)

if (ASIO_NET_ENABLE_SSL)
    target_compile_definitions(${PROJECT_NAME} INTERFACE -DASIO_NET_ENABLE_SSL)
endif ()

if (ASIO_NET_ENABLE_SSL)
    find_package(OpenSSL 1.1.0 REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE OpenSSL::SSL)
endif ()

if (ASIO_NET_BUILD_TEST)
    message(STATUS "ASIO_PATH: $ENV{ASIO_PATH}")
    include_directories($ENV{ASIO_PATH})
    link_libraries(${PROJECT_NAME})
    if (NOT MSVC)
        link_libraries(pthread)
    endif ()
    add_definitions(-DOPENSSL_PEM_PATH=\"${CMAKE_CURRENT_LIST_DIR}/test/ssl/\")

    # for github actions/ci
    if (ASIO_NET_DISABLE_ON_DATA_PRINT)
        add_definitions(-DASIO_NET_DISABLE_ON_DATA_PRINT)
    endif ()

    # for android standalone e.g. termux
    add_definitions(-DANDROID_STANDALONE)

    add_executable(${PROJECT_NAME}_test_version test/version.cpp)
    add_executable(${PROJECT_NAME}_test_compile_check test/compile_check/main.cpp test/compile_check/lib.cpp)
    add_executable(${PROJECT_NAME}_test_tcp test/tcp.cpp)
    add_executable(${PROJECT_NAME}_test_tcp_s test/tcp_s.cpp)
    add_executable(${PROJECT_NAME}_test_tcp_c test/tcp_c.cpp)
    add_executable(${PROJECT_NAME}_test_tcp_bigdata test/tcp_bigdata.cpp)
    add_executable(${PROJECT_NAME}_test_tcp_reconnect test/tcp_reconnect.cpp)
    add_executable(${PROJECT_NAME}_test_udp test/udp.cpp)
    add_executable(${PROJECT_NAME}_test_udp_s test/udp_s.cpp)
    add_executable(${PROJECT_NAME}_test_udp_c test/udp_c.cpp)
    add_executable(${PROJECT_NAME}_test_server_discovery test/server_discovery.cpp)
    add_executable(${PROJECT_NAME}_test_domain_tcp test/domain_tcp.cpp)
    add_executable(${PROJECT_NAME}_test_domain_udp test/domain_udp.cpp)
    if (ASIO_NET_ENABLE_SSL)
        add_executable(${PROJECT_NAME}_test_tcp_ssl_c test/tcp_ssl_c.cpp)
        add_executable(${PROJECT_NAME}_test_tcp_ssl_s test/tcp_ssl_s.cpp)
    endif ()

    add_compile_definitions(RPC_CORE_LOG_SHOW_DEBUG)
    add_compile_definitions(ASIO_NET_LOG_SHOW_DEBUG)
    add_executable(${PROJECT_NAME}_test_rpc test/rpc.cpp)
    add_executable(${PROJECT_NAME}_test_rpc_config test/rpc_config.cpp)
    add_executable(${PROJECT_NAME}_test_rpc_reconnect test/rpc_reconnect.cpp)
    add_executable(${PROJECT_NAME}_test_rpc_c_check_destroy test/rpc_c_check_destroy.cpp)
    add_executable(${PROJECT_NAME}_test_rpc_c_open_close test/rpc_c_open_close.cpp)
    add_executable(${PROJECT_NAME}_test_rpc_c_ping test/rpc_c_ping.cpp)
    add_executable(${PROJECT_NAME}_test_rpc_s test/rpc_s.cpp)
    add_executable(${PROJECT_NAME}_test_rpc_c test/rpc_c.cpp)
    add_executable(${PROJECT_NAME}_test_domain_rpc test/domain_rpc.cpp)
    add_executable(${PROJECT_NAME}_test_serial_port test/serial_port.cpp)
    if (ASIO_NET_ENABLE_SSL)
        add_executable(${PROJECT_NAME}_test_rpc_ssl test/rpc_ssl.cpp)
    endif ()
endif ()
