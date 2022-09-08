cmake_minimum_required(VERSION 3.1)

project(asio_net CXX)

option(asio_net_ENABLE_RPC "" ON)

option(RpcCore_BUILD_TEST "" OFF)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(RpcCore_BUILD_TEST ON)
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

if (RpcCore_BUILD_TEST)
    include_directories($ENV{ASIO_PATH})
    link_libraries(${PROJECT_NAME})

    add_executable(${PROJECT_NAME}_test_tcp test/tcp.cpp)
    add_executable(${PROJECT_NAME}_test_udp test/udp.cpp)
    add_executable(${PROJECT_NAME}_test_server_discovery test/server_discovery.cpp)

    if (asio_net_ENABLE_RPC)
        add_compile_definitions(RpcCore_LOG_SHOW_DEBUG)
        add_compile_definitions(asio_net_LOG_SHOW_DEBUG)
        add_executable(${PROJECT_NAME}_test_rpc test/rpc.cpp)
        add_executable(${PROJECT_NAME}_test_rpc_s test/rpc_s.cpp)
        add_executable(${PROJECT_NAME}_test_rpc_c test/rpc_c.cpp)
    endif ()
endif ()
