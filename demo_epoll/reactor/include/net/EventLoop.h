#pragma once

#include "net/Epoller.h"

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

class Channel;
class TcpConnection;

class EventLoop {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    bool isInLoopThread() const;

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    void addConnection(int fd);

private:
    void wakeup();
    void handleWakeup();
    void doPendingFunctors();
    void addConnectionInLoop(int fd);
    void removeConnection(int fd);
    void removeConnectionInLoop(int fd);

private:
    bool quit_;
    bool callingPendingFunctors_;
    const std::thread::id threadId_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, Channel*> channels_;
    std::unordered_map<int, std::unique_ptr<TcpConnection>> connections_;
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;
};
