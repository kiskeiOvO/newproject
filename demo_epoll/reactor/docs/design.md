# Reactor Layout

This directory is the modular version of the single-file `epoll7.cpp` demo.

Current split:

- `include/net/Buffer.h` and `src/Buffer.cpp`: byte buffer abstraction for input and output data.
- `include/net/Channel.h` and `src/Channel.cpp`: event interest registration and callback dispatch.
- `include/net/Epoller.h` and `src/Epoller.cpp`: thin wrapper over `epoll_create1`, `epoll_ctl`, and `epoll_wait`.
- `include/net/TcpConnection.h` and `src/TcpConnection.cpp`: one connection's lifecycle, buffers, and read/write handling.
- `include/net/EventLoop.h` and `src/EventLoop.cpp`: owns the poller, accepts new clients, and dispatches active events.
- `include/net/Socket.h` and `src/Socket.cpp`: basic socket operations and nonblocking setup.
- `examples/echo_server.cpp`: minimal executable wiring the pieces together.

The goal is to keep the original `epoll*.cpp` files as learning snapshots while using this directory as the base for future evolution.
