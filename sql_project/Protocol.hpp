#pragma once
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdexcept>
#include <cstdint>


class ProtocolException : public std::runtime_error {
public: using std::runtime_error::runtime_error;
};

inline void proto_write(int fd, const std::string& msg) {
    uint32_t len = (uint32_t)msg.size();
    size_t written = 0;
    const char* p = reinterpret_cast<const char*>(&len);
    while (written < sizeof(len)) {
        ssize_t r = write(fd, p+written, sizeof(len)-written);
        if (r<=0) throw ProtocolException("Write error (length)");
        written += r;
    }
    written = 0;
    while (written < len) {
        ssize_t r = write(fd, msg.c_str()+written, len-written);
        if (r<=0) throw ProtocolException("Write error (payload)");
        written += r;
    }
}

inline std::string proto_read(int fd) {
    uint32_t len = 0;
    size_t got = 0;
    char* p = reinterpret_cast<char*>(&len);
    while (got < sizeof(len)) {
        ssize_t r = read(fd, p+got, sizeof(len)-got);
        if (r<=0) throw ProtocolException("Read error (length)");
        got += r;
    }
    if (len == 0) return "";
    std::string buf(len, '\0');
    got = 0;
    while (got < len) {
        ssize_t r = read(fd, &buf[got], len-got);
        if (r<=0) throw ProtocolException("Read error (payload)");
        got += r;
    }
    return buf;
}
