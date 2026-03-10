#include "net/EventLoop.h"

#include "net/Channel.h"
#include "net/Epoller.h"
#include "net/Socket.h"
#include "net/TcpConnection.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <vector>
#include <unistd.h>

EventLoop::EventLoop(int listenfd)
    : listenfd_(listenfd),
      quit_(false),
      listenChannel_(new Channel(listenfd)),
      epoller_(new Epoller()) {
    listenChannel_->enableReading();
    listenChannel_->setReadCallback(std::bind(&EventLoop::handleNewConnection, this));

    channels_[listenfd_] = listenChannel_.get();
    epoller_->addChannel(listenChannel_.get());
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
    }
}

void EventLoop::handleNewConnection() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);

        int connfd = ::accept(listenfd_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        if (connfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            perror("accept");
            break;
        }

        if (Socket::setNonBlocking(connfd) == -1) {
            close(connfd);
            continue;
        }

        std::unique_ptr<TcpConnection> conn(new TcpConnection(
            connfd, std::bind(&EventLoop::updateChannel, this, std::placeholders::_1)));
        conn->setCloseCallback(std::bind(&EventLoop::removeConnection, this, std::placeholders::_1));

        channels_[connfd] = conn->channel();
        connections_[connfd] = std::move(conn);
        epoller_->addChannel(channels_[connfd]);

        char ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
        std::cout << "new client: " << ip
                  << ":" << ntohs(clientAddr.sin_port)
                  << ", fd=" << connfd << "\n";
    }
}

void EventLoop::removeConnection(int fd) {
    auto channelIt = channels_.find(fd);
    if (channelIt == channels_.end()) {
        return;
    }

    epoller_->removeChannel(channelIt->second);
    close(fd);
    channels_.erase(channelIt);
    connections_.erase(fd);
}

void EventLoop::updateChannel(Channel* channel) {
    epoller_->updateChannel(channel);
}
