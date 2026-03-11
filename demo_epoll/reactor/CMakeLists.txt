cmake_minimum_required(VERSION 3.16)
project(epoll_reactor_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(epoll_reactor
    src/Acceptor.cpp
    src/Buffer.cpp
    src/Channel.cpp
    src/Epoller.cpp
    src/EventLoop.cpp
    src/EventLoopThread.cpp
    src/EventLoopThreadPool.cpp
    src/Socket.cpp
    src/TcpConnection.cpp
    src/TcpServer.cpp
)

target_include_directories(epoll_reactor PUBLIC include)
target_link_libraries(epoll_reactor PUBLIC pthread)

add_executable(echo_server examples/echo_server.cpp)
target_link_libraries(echo_server PRIVATE epoll_reactor)
