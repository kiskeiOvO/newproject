#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <netinet/in.h>

class Channel;
class EventLoop;
class Socket;

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(int, const sockaddr_in&)>;

    Acceptor(EventLoop* loop, uint16_t port, int backlog);
    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback cb);
    void listen();

private:
    void handleRead();

private:
    EventLoop* loop_;
    std::unique_ptr<Socket> listenSocket_;
    std::unique_ptr<Channel> listenChannel_;
    NewConnectionCallback newConnectionCallback_;
};
