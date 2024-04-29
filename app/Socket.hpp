#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

namespace platform {

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

    class UDPSocket {
    protected:
        mutable std::mutex lock;

    public:
        using ClientID = uint32_t;
    
    protected:
        static ClientID next_free_id;

        const uint16_t port;
    
    public:
        UDPSocket(uint16_t port) : port{port} {}
        
        virtual ~UDPSocket() = default;

        virtual bool IsOpen() const = 0;

        virtual void Send(const void* data, size_t size, ClientID to = 0) const = 0;
        virtual size_t Recv(void* buffer, size_t max_size, ClientID& from) const = 0;

        static std::shared_ptr<UDPSocket> CreateServer(uint16_t port);
        static std::shared_ptr<UDPSocket> CreateClient(const std::string& address, uint16_t port);
    };

}

#endif