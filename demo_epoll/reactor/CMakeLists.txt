cmake_minimum_required(VERSION 3.16)
project(epoll_reactor_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(epoll_reactor
    src/Buffer.cpp
    src/Channel.cpp
    src/Epoller.cpp
    src/EventLoop.cpp
    src/Socket.cpp
    src/TcpConnection.cpp
)

target_include_directories(epoll_reactor PUBLIC include)

add_executable(echo_server examples/echo_server.cpp)
target_link_libraries(echo_server PRIVATE epoll_reactor)
