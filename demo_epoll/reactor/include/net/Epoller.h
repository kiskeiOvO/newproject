#pragma once

#include <sys/epoll.h>

#include <vector>

class Channel;

class Epoller {
public:
    Epoller();
    ~Epoller();

    void addChannel(Channel* channel);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    int wait(std::vector<epoll_event>& events);

private:
    int epfd_;
};
