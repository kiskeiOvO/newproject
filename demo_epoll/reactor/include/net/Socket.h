#pragma once

#include <cstdint>

class Socket {
public:
    explicit Socket(int fd);
    ~Socket();

    int fd() const;

    void bindAddress(uint16_t port);
    void listen(int backlog);

    static int setNonBlocking(int fd);

private:
    int fd_;
};
