#pragma once

#include "net/Channel.h"
#include "net/Epoller.h"
#include "net/TcpConnection.h"

#include <memory>
#include <unordered_map>

class EventLoop {
public:
    explicit EventLoop(int listenfd);
    void loop();

private:
    void handleNewConnection();
    void removeConnection(int fd);
    void updateChannel(Channel* channel);

private:
    int listenfd_;
    bool quit_;
    std::unique_ptr<Channel> listenChannel_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, Channel*> channels_;
    std::unordered_map<int, std::unique_ptr<TcpConnection>> connections_;
};
