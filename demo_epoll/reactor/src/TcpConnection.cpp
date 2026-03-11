#include "net/TcpConnection.h"

#include "net/Channel.h"
#include "net/EventLoop.h"

#include <cerrno>
#include <cstdio>
#include <functional>
#include <iostream>
#include <unistd.h>

namespace {
const int kBufferSize = 1024;
}

TcpConnection::TcpConnection(int fd, EventLoop* loop)
    : fd_(fd),
      loop_(loop),
      channel_(new Channel(fd)) {
    channel_->enableReading();
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

int TcpConnection::fd() const {
    return fd_;
}

Channel* TcpConnection::channel() const {
    return channel_.get();
}

void TcpConnection::setCloseCallback(CloseCallback cb) {
    closeCallback_ = std::move(cb);
}

void TcpConnection::handleRead() {
    char buf[kBufferSize];

    while (true) {
        ssize_t n = ::read(fd_, buf, sizeof(buf));
        if (n > 0) {
            inputBuffer_.append(buf, static_cast<size_t>(n));
        } else if (n == 0) {
            std::cout << "client fd " << fd_ << " closed\n";
            handleClose();
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            perror("read");
            handleClose();
            break;
        }
    }

    if (!inputBuffer_.empty()) {
        std::string message = inputBuffer_.retrieveAllAsString();
        std::cout << "fd " << fd_ << " says: " << message << "\n";
        send(message);
    }
}

void TcpConnection::handleWrite() {
    while (!outputBuffer_.empty()) {
        ssize_t n = ::write(fd_, outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(static_cast<size_t>(n));
        } else if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            perror("write");
            handleClose();
            return;
        }
    }

    if (outputBuffer_.empty() && channel_->isWriting()) {
        channel_->disableWriting();
        updateChannel();
    }
}

void TcpConnection::handleClose() {
    if (closeCallback_) {
        closeCallback_(fd_);
    }
}

void TcpConnection::handleError() {
    std::cerr << "fd " << fd_ << " error event\n";
    handleClose();
}

void TcpConnection::send(const std::string& data) {
    outputBuffer_.append(data);

    if (!channel_->isWriting()) {
        channel_->enableWriting();
        updateChannel();
    }

    handleWrite();
}

void TcpConnection::updateChannel() {
    if (loop_ != nullptr) {
        loop_->updateChannel(channel());
    }
}
