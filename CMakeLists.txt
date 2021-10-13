cmake_minimum_required(VERSION 3.1)

project(asio_net CXX)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

include_directories(. $ENV{ASIO_PATH})
add_executable(${PROJECT_NAME}_test_tcp test/tcp.cpp)
add_executable(${PROJECT_NAME}_test_udp test/udp.cpp)
