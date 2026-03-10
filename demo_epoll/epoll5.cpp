#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cerrno>
#include <cstring>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <functional>

static const int kPort = 8888;
static const int kMaxEvents = 1024;
static const int kListenBacklog = 10;
static const int kBufferSize = 1024;

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}

class Socket {
public:
    explicit Socket(int fd) : fd_(fd) {}

    ~Socket() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    int fd() const {
        return fd_;
    }

    void bindAddress() {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(kPort);

        int opt = 1;
        if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            perror("setsockopt SO_REUSEADDR");
            throw std::runtime_error("setsockopt failed");
        }

        if (bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
            perror("bind");
            throw std::runtime_error("bind failed");
        }
    }

    void listen() {
        if (::listen(fd_, kListenBacklog) == -1) {
            perror("listen");
            throw std::runtime_error("listen failed");
        }
    }

private:
    int fd_;
};

class Channel {
public:
    using EventCallback = std::function<void()>;

    explicit Channel(int fd)
        : fd_(fd), events_(0), revents_(0) {}

    int fd() const {
        return fd_;
    }

    uint32_t events() const {
        return events_;
    }

    void setRevents(uint32_t revents) {
        revents_ = revents;
    }

    void enableReading() {
        events_ |= EPOLLIN;
    }

    void setReadCallback(EventCallback cb) {
        readCallback_ = std::move(cb);
    }

    void handleEvent() {
        if (revents_ & EPOLLIN) {
            if (readCallback_) {
                readCallback_();
            }
        }
    }

private:
    int fd_;
    uint32_t events_;
    uint32_t revents_;
    EventCallback readCallback_;
};

class TcpConnection {
public:
    using CloseCallback = std::function<void(int)>;

    explicit TcpConnection(int fd)
        : fd_(fd), channel_(new Channel(fd)) {
        channel_->enableReading();
        channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    }

    ~TcpConnection() {
        delete channel_;
    }

    int fd() const {
        return fd_;
    }

    Channel* channel() const {
        return channel_;
    }

    void setCloseCallback(CloseCallback cb) {
        closeCallback_ = std::move(cb);
    }

private:
    void handleRead() {
        char buf[kBufferSize];

        while (true) {
            ssize_t n = ::read(fd_, buf, sizeof(buf) - 1);

            if (n > 0) {
                buf[n] = '\0';
                std::cout << "fd " << fd_ << " says: " << buf << "\n";

                ssize_t wn = ::write(fd_, buf, n);
                if (wn == -1) {
                    perror("write");
                    handleClose();
                    break;
                }
            } else if (n == 0) {
                std::cout << "client fd " << fd_ << " closed\n";
                handleClose();
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    perror("read");
                    handleClose();
                    break;
                }
            }
        }
    }

    void handleClose() {
        if (closeCallback_) {
            closeCallback_(fd_);
        }
    }

private:
    int fd_;
    Channel* channel_;
    CloseCallback closeCallback_;
};

class Epoller {
public:
    Epoller() : epfd_(epoll_create1(0)) {
        if (epfd_ == -1) {
            perror("epoll_create1");
            throw std::runtime_error("epoll_create1 failed");
        }
    }

    ~Epoller() {
        if (epfd_ >= 0) {
            close(epfd_);
        }
    }

    void addChannel(Channel* channel) {
        epoll_event ev{};
        ev.events = channel->events();
        ev.data.fd = channel->fd();

        if (epoll_ctl(epfd_, EPOLL_CTL_ADD, channel->fd(), &ev) == -1) {
            perror("epoll_ctl ADD");
            throw std::runtime_error("epoll_ctl add failed");
        }
    }

    void updateChannel(Channel* channel) {
        epoll_event ev{};
        ev.events = channel->events();
        ev.data.fd = channel->fd();

        if (epoll_ctl(epfd_, EPOLL_CTL_MOD, channel->fd(), &ev) == -1) {
            perror("epoll_ctl MOD");
            throw std::runtime_error("epoll_ctl mod failed");
        }
    }

    void removeChannel(Channel* channel) {
        if (epoll_ctl(epfd_, EPOLL_CTL_DEL, channel->fd(), nullptr) == -1) {
            perror("epoll_ctl DEL");
        }
    }

    int wait(std::vector<epoll_event>& events) {
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

private:
    int epfd_;
};

class EventLoop {
public:
    explicit EventLoop(int listenfd)
        : listenfd_(listenfd), quit_(false) {
        Channel* listenChannel = new Channel(listenfd_);
        listenChannel->enableReading();
        listenChannel->setReadCallback(std::bind(&EventLoop::handleNewConnection, this));

        channels_[listenfd_] = listenChannel;
        epoller_.addChannel(listenChannel);
    }

    ~EventLoop() {
        for (auto& pair : channels_) {
            if (pair.first == listenfd_) {
                delete pair.second;
            }
        }

        for (auto& pair : connections_) {
            delete pair.second;
        }
    }

    void loop() {
        std::vector<epoll_event> activeEvents;

        while (!quit_) {
            int n = epoller_.wait(activeEvents);

            for (int i = 0; i < n; ++i) {
                int fd = activeEvents[i].data.fd;

                auto it = channels_.find(fd);
                if (it == channels_.end()) {
                    continue;
                }

                Channel* ch = it->second;
                ch->setRevents(activeEvents[i].events);
                ch->handleEvent();
            }
        }
    }

private:
    void handleNewConnection() {
        while (true) {
            sockaddr_in clientAddr{};
            socklen_t len = sizeof(clientAddr);

            int connfd = ::accept(listenfd_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
            if (connfd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    perror("accept");
                    break;
                }
            }

            if (setNonBlocking(connfd) == -1) {
                close(connfd);
                continue;
            }

            TcpConnection* conn = new TcpConnection(connfd);
            conn->setCloseCallback(
                std::bind(&EventLoop::removeConnection, this, std::placeholders::_1));

            connections_[connfd] = conn;
            channels_[connfd] = conn->channel();
            epoller_.addChannel(conn->channel());

            char ip[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
            std::cout << "new client: " << ip
                      << ":" << ntohs(clientAddr.sin_port)
                      << ", fd=" << connfd << "\n";
        }
    }

    void removeConnection(int fd) {
        auto chIt = channels_.find(fd);
        if (chIt == channels_.end()) {
            return;
        }

        Channel* ch = chIt->second;
        epoller_.removeChannel(ch);
        close(fd);
        channels_.erase(chIt);

        auto connIt = connections_.find(fd);
        if (connIt != connections_.end()) {
            delete connIt->second;
            connections_.erase(connIt);
        }
    }

private:
    int listenfd_;
    Epoller epoller_;
    bool quit_;
    std::unordered_map<int, Channel*> channels_;
    std::unordered_map<int, TcpConnection*> connections_;
};

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        return 1;
    }

    if (setNonBlocking(listenfd) == -1) {
        close(listenfd);
        return 1;
    }

    try {
        Socket listenSocket(listenfd);
        listenSocket.bindAddress();
        listenSocket.listen();

        std::cout << "epoll server listening on " << kPort << "...\n";

        EventLoop loop(listenSocket.fd());
        loop.loop();
    } catch (const std::exception& ex) {
        std::cerr << "fatal error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}