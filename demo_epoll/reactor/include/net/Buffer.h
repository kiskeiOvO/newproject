#pragma once

#include <cstddef>
#include <string>

class Buffer {
public:
    void append(const char* data, size_t len);
    void append(const std::string& data);

    const char* peek() const;
    size_t readableBytes() const;
    bool empty() const;

    void retrieve(size_t len);
    std::string retrieveAllAsString();

private:
    std::string buffer_;
};
