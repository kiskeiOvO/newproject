#pragma once

#include <memory>
#include <netinet/in.h>

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer {
public:
    TcpServer(EventLoop* loop, uint16_t port, int backlog, int workerThreads);
    ~TcpServer();

    void start();

private:
    void handleNewConnection(int fd, const sockaddr_in& peerAddr);

private:
    EventLoop* loop_;
    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;
    bool started_;
};
