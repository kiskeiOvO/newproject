#include "net/EventLoop.h"

#include "net/Channel.h"
#include "net/TcpConnection.h"

#include <cstdint>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <sys/eventfd.h>
#include <unistd.h>
#include <vector>

namespace {
int createEventfd() {
    int fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd == -1) {
        perror("eventfd");
        throw std::runtime_error("eventfd failed");
    }
    return fd;
}
}

EventLoop::EventLoop()
    : quit_(false),
      callingPendingFunctors_(false),
      threadId_(std::this_thread::get_id()),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(wakeupFd_)),
      epoller_(new Epoller()) {
    wakeupChannel_->enableReading();
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleWakeup, this));

    channels_[wakeupFd_] = wakeupChannel_.get();
    epoller_->addChannel(wakeupChannel_.get());
}

EventLoop::~EventLoop() {
    channels_.erase(wakeupFd_);
    wakeupChannel_.reset();
    if (wakeupFd_ >= 0) {
        close(wakeupFd_);
    }
}

void EventLoop::loop() {
    std::vector<epoll_event> activeEvents;

    while (!quit_) {
        int n = epoller_->wait(activeEvents);
        for (int i = 0; i < n; ++i) {
            int fd = activeEvents[i].data.fd;
            auto it = channels_.find(fd);
            if (it == channels_.end()) {
                continue;
            }

            Channel* channel = it->second;
            channel->setRevents(activeEvents[i].events);
            channel->handleEvent();
        }

        doPendingFunctors();
    }
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
        return;
    }

    queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

bool EventLoop::isInLoopThread() const {
    return threadId_ == std::this_thread::get_id();
}

void EventLoop::updateChannel(Channel* channel) {
    if (channels_.find(channel->fd()) == channels_.end()) {
        channels_[channel->fd()] = channel;
        epoller_->addChannel(channel);
        return;
    }

    epoller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    auto it = channels_.find(channel->fd());
    if (it == channels_.end()) {
        return;
    }

    epoller_->removeChannel(channel);
    channels_.erase(it);
}

void EventLoop::addConnection(int fd) {
    runInLoop(std::bind(&EventLoop::addConnectionInLoop, this, fd));
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != static_cast<ssize_t>(sizeof(one))) {
        perror("write wakeupFd");
    }
}

void EventLoop::handleWakeup() {
    uint64_t one = 0;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != static_cast<ssize_t>(sizeof(one))) {
        perror("read wakeupFd");
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i) {
        functors[i]();
    }

    callingPendingFunctors_ = false;
}

void EventLoop::addConnectionInLoop(int fd) {
    std::unique_ptr<TcpConnection> conn(new TcpConnection(fd, this));
    conn->setCloseCallback(std::bind(&EventLoop::removeConnection, this, std::placeholders::_1));

    Channel* channel = conn->channel();
    connections_[fd] = std::move(conn);
    updateChannel(channel);
}

void EventLoop::removeConnection(int fd) {
    runInLoop(std::bind(&EventLoop::removeConnectionInLoop, this, fd));
}

void EventLoop::removeConnectionInLoop(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        return;
    }

    removeChannel(it->second->channel());
    close(fd);
    connections_.erase(it);
}
