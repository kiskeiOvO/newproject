#include "net/Channel.h"

#include <sys/epoll.h>

Channel::Channel(int fd)
    : fd_(fd), events_(0), revents_(0) {}

int Channel::fd() const {
    return fd_;
}

uint32_t Channel::events() const {
    return events_;
}

void Channel::setRevents(uint32_t revents) {
    revents_ = revents;
}

void Channel::enableReading() {
    events_ |= EPOLLIN;
}

void Channel::enableWriting() {
    events_ |= EPOLLOUT;
}

void Channel::disableWriting() {
    events_ &= ~EPOLLOUT;
}

bool Channel::isWriting() const {
    return (events_ & EPOLLOUT) != 0;
}

void Channel::setReadCallback(EventCallback cb) {
    readCallback_ = std::move(cb);
}

void Channel::setWriteCallback(EventCallback cb) {
    writeCallback_ = std::move(cb);
}

void Channel::setCloseCallback(EventCallback cb) {
    closeCallback_ = std::move(cb);
}

void Channel::setErrorCallback(EventCallback cb) {
    errorCallback_ = std::move(cb);
}

void Channel::handleEvent() {
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) {
            readCallback_();
        }
    }

    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}
