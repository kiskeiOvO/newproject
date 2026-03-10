#pragma once

#include <cstdint>
#include <functional>

class Channel {
public:
    using EventCallback = std::function<void()>;

    explicit Channel(int fd);

    int fd() const;
    uint32_t events() const;
    void setRevents(uint32_t revents);

    void enableReading();
    void enableWriting();
    void disableWriting();
    bool isWriting() const;

    void setReadCallback(EventCallback cb);
    void setWriteCallback(EventCallback cb);
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);

    void handleEvent();

private:
    int fd_;
    uint32_t events_;
    uint32_t revents_;
    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
