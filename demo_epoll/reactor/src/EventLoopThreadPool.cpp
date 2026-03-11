#include "net/EventLoopThreadPool.h"

#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
/*
                 ┌──────────────┐
                 │   mainLoop   │
                 │   (accept)   │
                 └──────┬───────┘
                        │
                 新连接 connfd
                        │
                        ▼
              EventLoopThreadPool
                        │
       ┌───────────┬───────────┬───────────┐
       ▼           ▼           ▼
   EventLoop1   EventLoop2   EventLoop3
   (thread1)    (thread2)    (thread3)
       │           │           │
      epoll       epoll       epoll

*/
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
    : baseLoop_(baseLoop),//主线程的 EventLoop（一般负责 accept 新连接）
      numThreads_(numThreads),//工作线程数量
      next_(0),//轮询索引
      started_(false) {}//是否已经启动线程池

EventLoopThreadPool::~EventLoopThreadPool() = default;

/* start 创建线程池
EventLoopThreadPool
      │
      ├── thread1 → EventLoop1
      ├── thread2 → EventLoop2
      ├── thread3 → EventLoop3
*/
void EventLoopThreadPool::start() {
    if (started_) {
        return;
    }

    started_ = true;
    for (int i = 0; i < numThreads_; ++i) {
        std::unique_ptr<EventLoopThread> thread(new EventLoopThread());
        EventLoop* loop = thread->startLoop();
        threads_.push_back(std::move(thread));
        loops_.push_back(loop);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    if (loops_.empty()) {
        return baseLoop_;
    }

    EventLoop* loop = loops_[next_];
    ++next_;
    if (next_ >= static_cast<int>(loops_.size())) {
        next_ = 0;
    }
    return loop;
}
