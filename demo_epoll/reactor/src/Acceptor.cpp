#include "net/Acceptor.h"

#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/Socket.h"

#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Acceptor::Acceptor(EventLoop* loop, uint16_t port, int backlog)
    : loop_(loop),
      listenSocket_(new Socket(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0))),
      listenChannel_(new Channel(listenSocket_->fd())) {
    if (listenSocket_->fd() == -1) {
        perror("socket");
        throw std::runtime_error("socket failed");
    }

    listenSocket_->bindAddress(port);
    listenSocket_->listen(backlog);

    listenChannel_->enableReading();
    listenChannel_->setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() = default;

void Acceptor::setNewConnectionCallback(NewConnectionCallback cb) {
    newConnectionCallback_ = std::move(cb);
}

void Acceptor::listen() {
    loop_->updateChannel(listenChannel_.get());
}

void Acceptor::handleRead() {
    while (true) {
        sockaddr_in peerAddr{};
        socklen_t len = sizeof(peerAddr);
        int connfd = ::accept4(
            listenSocket_->fd(),
            reinterpret_cast<sockaddr*>(&peerAddr),
            &len,
            SOCK_NONBLOCK | SOCK_CLOEXEC);

        if (connfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            perror("accept4");
            break;
        }

        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
            continue;
        }

        close(connfd);
    }
}
