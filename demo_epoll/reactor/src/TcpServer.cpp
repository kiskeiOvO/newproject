#include "net/TcpServer.h"

#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"

#include <arpa/inet.h>
#include <functional>
#include <iostream>

TcpServer::TcpServer(EventLoop* loop, uint16_t port, int backlog, int workerThreads)
    : loop_(loop),
      acceptor_(new Acceptor(loop, port, backlog)),
      threadPool_(new EventLoopThreadPool(loop, workerThreads)),
      started_(false) {
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::handleNewConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() = default;

void TcpServer::start() {
    if (started_) {
        return;
    }

    started_ = true;
    threadPool_->start();
    acceptor_->listen();
}

void TcpServer::handleNewConnection(int fd, const sockaddr_in& peerAddr) {
    EventLoop* ioLoop = threadPool_->getNextLoop();
    ioLoop->addConnection(fd);

    char ip[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &peerAddr.sin_addr, ip, sizeof(ip));
    std::cout << "new client: " << ip
              << ":" << ntohs(peerAddr.sin_port)
              << ", fd=" << fd
              << ", assigned to worker loop\n";
}
