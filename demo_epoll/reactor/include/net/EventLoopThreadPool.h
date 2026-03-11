#pragma once

#include <memory>
#include <vector>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool {
public:
    EventLoopThreadPool(EventLoop* baseLoop, int numThreads);
    ~EventLoopThreadPool();

    void start();
    EventLoop* getNextLoop();

private:
    EventLoop* baseLoop_;
    int numThreads_;
    int next_;
    bool started_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};
