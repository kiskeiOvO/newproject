#pragma once

#include <functional>
#include <memory>
#include <string>

#include "net/Buffer.h"

class Channel;

class TcpConnection {
public:
    using CloseCallback = std::function<void(int)>;
    using UpdateCallback = std::function<void(Channel*)>;

    TcpConnection(int fd, UpdateCallback updateCallback);

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
    std::unique_ptr<Channel> channel_;
    CloseCallback closeCallback_;
    UpdateCallback updateCallback_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};
