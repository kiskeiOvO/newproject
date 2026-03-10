#include "net/Epoller.h"

#include "net/Channel.h"

#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <unistd.h>

namespace {
const int kMaxEvents = 1024;
}

Epoller::Epoller() : epfd_(epoll_create1(0)) {
    if (epfd_ == -1) {
        perror("epoll_create1");
        throw std::runtime_error("epoll_create1 failed");
    }
}

Epoller::~Epoller() {
    if (epfd_ >= 0) {
        close(epfd_);
    }
}

void Epoller::addChannel(Channel* channel) {
    epoll_event ev{};
    ev.events = channel->events();
    ev.data.fd = channel->fd();

    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, channel->fd(), &ev) == -1) {
        perror("epoll_ctl ADD");
        throw std::runtime_error("epoll_ctl add failed");
    }
}

void Epoller::updateChannel(Channel* channel) {
    epoll_event ev{};
    ev.events = channel->events();
    ev.data.fd = channel->fd();

    if (epoll_ctl(epfd_, EPOLL_CTL_MOD, channel->fd(), &ev) == -1) {
        perror("epoll_ctl MOD");
        throw std::runtime_error("epoll_ctl mod failed");
    }
}

void Epoller::removeChannel(Channel* channel) {
    if (epoll_ctl(epfd_, EPOLL_CTL_DEL, channel->fd(), nullptr) == -1) {
        perror("epoll_ctl DEL");
    }
}

int Epoller::wait(std::vector<epoll_event>& events) {
    events.resize(kMaxEvents);

    int n = epoll_wait(epfd_, events.data(), static_cast<int>(events.size()), -1);
    if (n == -1) {
        if (errno == EINTR) {
            return 0;
        }

        perror("epoll_wait");
        throw std::runtime_error("epoll_wait failed");
    }

    events.resize(n);
    return n;
}
