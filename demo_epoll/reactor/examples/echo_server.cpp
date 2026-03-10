#include "net/EventLoop.h"
#include "net/Socket.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace {
const uint16_t kPort = 8888;
const int kListenBacklog = 10;
}

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        return 1;
    }

    if (Socket::setNonBlocking(listenfd) == -1) {
        close(listenfd);
        return 1;
    }

    try {
        Socket listenSocket(listenfd);
        listenSocket.bindAddress(kPort);
        listenSocket.listen(kListenBacklog);

        std::cout << "epoll server listening on " << kPort << "...\n";

        EventLoop loop(listenSocket.fd());
        loop.loop();
    } catch (const std::exception& ex) {
        std::cerr << "fatal error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
