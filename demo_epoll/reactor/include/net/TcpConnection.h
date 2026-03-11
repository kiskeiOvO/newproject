#pragma once

#include "net/Buffer.h"

#include <functional>
#include <memory>
#include <string>

class Channel;
class EventLoop;

class TcpConnection {
public:
    using CloseCallback = std::function<void(int)>;

    TcpConnection(int fd, EventLoop* loop);

    int fd() const;
    Channel* channel() const;

    void setCloseCallback(CloseCallback cb);

private:
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    void send(const std::string& data);
    void updateChannel();

private:
    int fd_;
    EventLoop* loop_;
    std::unique_ptr<Channel> channel_;
    CloseCallback closeCallback_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};
