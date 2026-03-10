#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cerrno>
#include <cstring>
#include <iostream>

static int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind");
        close(listenfd);
        return 1;
    }

    if (listen(listenfd, 10) == -1) {
        perror("listen");
        close(listenfd);
        return 1;
    }

    if (setNonBlocking(listenfd) == -1) {
        perror("setNonBlocking listenfd");
        close(listenfd);
        return 1;
    }

    // 1. 创建 epoll 实例
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        close(listenfd);
        return 1;
    }

    // 2. 把监听 fd 加入 epoll
    epoll_event ev{};
    ev.events = EPOLLIN;      // 关心“可读”事件
    ev.data.fd = listenfd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl add listenfd");
        close(epfd);
        close(listenfd);
        return 1;
    }

    std::cout << "epoll server listening on 8888...\n";

    epoll_event events[1024];

    while (true) {
        // 3. 等待事件发生
        int nfds = epoll_wait(epfd, events, 1024, -1);//epoll_wait 返回一批有事件的 fd,存在events数组里，共有nfds个有事件的fd
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            // 4. 监听 fd 有事件：说明有新连接来了
            if (fd == listenfd) {
                while (true) {
                    sockaddr_in clientAddr{};
                    socklen_t clientLen = sizeof(clientAddr);

                    int connfd = accept(listenfd, (sockaddr*)&clientAddr, &clientLen);
                    if (connfd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 已经没有更多连接可 accept
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }

                    std::cout << "New client: "
                              << inet_ntoa(clientAddr.sin_addr)
                              << ":" << ntohs(clientAddr.sin_port)
                              << ", fd=" << connfd << "\n";

                    setNonBlocking(connfd);

                    epoll_event clientEv{};
                    clientEv.events = EPOLLIN;
                    clientEv.data.fd = connfd;

                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &clientEv) == -1) {
                        perror("epoll_ctl add connfd");
                        close(connfd);
                    }
                }
            }
            // 5. 普通连接 fd 有事件：说明该连接可读
            else if (events[i].events & EPOLLIN) {
                char buf[1024];

                while (true) {
                    ssize_t n = read(fd, buf, sizeof(buf) - 1);

                    if (n > 0) {
                        buf[n] = '\0';
                        std::cout << "fd " << fd << " says: " << buf << "\n";

                        // echo 回去
                        write(fd, buf, n);
                    } else if (n == 0) {
                        std::cout << "client fd " << fd << " closed\n";
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                        break;
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 当前数据读完了
                            break;
                        } else {
                            perror("read");
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                            close(fd);
                            break;
                        }
                    }
                }
            }
        }
    }

    close(epfd);
    close(listenfd);
    return 0;
}