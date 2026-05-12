#ifndef SOCK_WRAP_H
#define SOCK_WRAP_H
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <stdexcept>
#include <cstring>

namespace SQL {
class SocketException : public std::runtime_error {
public:
    explicit SocketException(const std::string& msg) : std::runtime_error(msg) {}
};

class ServerSocket {
public:
    explicit ServerSocket(const std::string& path) {
        unlink(path.c_str());
        fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_ < 0) throw SocketException("socket() failed");
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        if (bind(fd_, (sockaddr*)&addr, sizeof(addr)) < 0)
            throw SocketException("bind() failed");
        if (listen(fd_, 5) < 0) throw SocketException("listen() failed");
    }
    ~ServerSocket() { close(fd_); }
    int acceptConnection() {
        int client = accept(fd_, nullptr, nullptr);
        if (client < 0) throw SocketException("accept() failed");
        return client;
    }
private:
    int fd_;
};

class ClientSocket {
public:
    explicit ClientSocket(const std::string& path) {
        fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_ < 0) throw SocketException("socket() failed");
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        if (connect(fd_, (sockaddr*)&addr, sizeof(addr)) < 0)
            throw SocketException("connect() failed");
    }
    ~ClientSocket() { close(fd_); }
    void sendString(const std::string& s) {
        std::string packet = s + "\n";
        if (write(fd_, packet.c_str(), packet.size()) <= 0)
            throw SocketException("write() failed");
    }
    std::string receiveString() {
        char buf[4096];
        ssize_t n = read(fd_, buf, sizeof(buf) - 1);
        if (n <= 0) throw SocketException("read() failed or connection closed");
        buf[n] = '\0';
        std::string res(buf);
        if (!res.empty() && res.back() == '\n') res.pop_back();
        return res;
    }
private:
    int fd_;
};
} // namespace SQL
#endif
