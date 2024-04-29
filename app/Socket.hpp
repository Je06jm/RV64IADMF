#ifndef APP_SOCKET_HPP
#define APP_SOCKET_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

class TCPSocket {
protected:
    mutable std::mutex lock;

public:
    virtual ~TCPSocket() = default;

    virtual bool IsOpen() const = 0;
    virtual bool IsServer() const = 0;

    virtual void Send(const void* data, size_t size) const = 0;
    virtual size_t Recv(void* buffer, size_t max_size) const = 0;

    virtual std::shared_ptr<TCPSocket> Accept() const = 0;

    static std::shared_ptr<TCPSocket> CreateServer(uint16_t port);
    static std::shared_ptr<TCPSocket> CreateClient(const std::string& address, uint16_t port);
};

#endif