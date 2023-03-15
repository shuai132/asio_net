cmake_minimum_required(VERSION 3.1)

project(asio_net CXX)

option(asio_net_ENABLE_RPC "" ON)

option(asio_net_BUILD_TEST "" OFF)

option(asio_net_DISABLE_ON_DATA_PRINT "" OFF)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(asio_net_BUILD_TEST ON)
endif ()

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE .)

if (asio_net_ENABLE_RPC)
    target_include_directories(${PROJECT_NAME} INTERFACE
            rpc/RpcCore
            rpc
            )
endif ()

if (asio_net_BUILD_TEST)
    message(STATUS "ASIO_PATH: $ENV{ASIO_PATH}")
    include_directories($ENV{ASIO_PATH})
    link_libraries(${PROJECT_NAME})
    link_libraries(pthread)

    # for github actions/ci
    if (asio_net_DISABLE_ON_DATA_PRINT)
        add_definitions(-Dasio_net_DISABLE_ON_DATA_PRINT)
    endif ()

    add_executable(${PROJECT_NAME}_test_tcp test/tcp.cpp)
    add_executable(${PROJECT_NAME}_test_udp test/udp.cpp)
    add_executable(${PROJECT_NAME}_test_server_discovery test/server_discovery.cpp)

    if (asio_net_ENABLE_RPC)
        add_compile_definitions(RpcCore_LOG_SHOW_DEBUG)
        add_compile_definitions(asio_net_LOG_SHOW_DEBUG)
        add_executable(${PROJECT_NAME}_test_rpc test/rpc.cpp)
        add_executable(${PROJECT_NAME}_test_rpc_s test/rpc_s.cpp)
        add_executable(${PROJECT_NAME}_test_rpc_c test/rpc_c.cpp)
        add_executable(${PROJECT_NAME}_test_tcp_s test/tcp_s.cpp)
        add_executable(${PROJECT_NAME}_test_tcp_c test/tcp_c.cpp)
        add_executable(${PROJECT_NAME}_test_domain_tcp test/domain_tcp.cpp)
        add_executable(${PROJECT_NAME}_test_domain_rpc test/domain_rpc.cpp)
        add_executable(${PROJECT_NAME}_test_domain_udp test/domain_udp.cpp)
    endif ()
endif ()
