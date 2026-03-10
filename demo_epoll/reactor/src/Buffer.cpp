#include "net/Buffer.h"

void Buffer::append(const char* data, size_t len) {
    buffer_.append(data, len);
}

void Buffer::append(const std::string& data) {
    buffer_.append(data);
}

const char* Buffer::peek() const {
    return buffer_.data();
}

size_t Buffer::readableBytes() const {
    return buffer_.size();
}

bool Buffer::empty() const {
    return buffer_.empty();
}

void Buffer::retrieve(size_t len) {
    if (len >= buffer_.size()) {
        buffer_.clear();
        return;
    }

    buffer_.erase(0, len);
}

std::string Buffer::retrieveAllAsString() {
    std::string data = buffer_;
    buffer_.clear();
    return data;
}
