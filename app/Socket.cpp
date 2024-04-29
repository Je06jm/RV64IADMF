#include "socket.hpp"

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#endif

#include <iostream>
#include <mutex>
#include <unordered_map>

#if defined(_WIN32) || defined(_WIN64)
uint32_t users = 0;
std::mutex users_lock;

UDPSocket::ClientID UDPSocket::next_free_id = 1;

class WindowsWSAUser {
public:
    WindowsWSAUser() {
        users_lock.lock();
        if (users == 0) {
            WSAData wsa_data;
            int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);

            if (result != 0) {
                std::cerr << "Cannot start WSA" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }

        users++;
        users_lock.unlock();
    }

    virtual ~WindowsWSAUser() {
        users_lock.lock();
        users--;

        if (users == 0) {
            WSACleanup();
        }

        users_lock.unlock();
    };
};

class WindowsTCPSocket : public TCPSocket, public WindowsWSAUser {
protected:
    mutable SOCKET handle = INVALID_SOCKET;

    virtual int Bind(const std::string& address, uint16_t port, uint16_t family) const = 0;

    void Close() const {
        lock.lock();
        if (handle != INVALID_SOCKET) {
            closesocket(handle);
            handle = INVALID_SOCKET;
        }
        lock.unlock();
    }

public:
    WindowsTCPSocket() : WindowsWSAUser() {};

    ~WindowsTCPSocket() {
        Close();
    }

    void Setup(const std::string& address, uint16_t port) {
        uint16_t family;
        if (address.find(":") == std::string::npos) {
            family = AF_INET;
        } else {
            family = AF_INET6;
        }

        handle = socket(family, SOCK_STREAM, 0);

        if (handle == INVALID_SOCKET) {
            return;
        }

        int err = Bind(address, port, family);

        if (err == SOCKET_ERROR) {
            Close();
            return;
        }

        if (IsServer()) {
            err = listen(handle, SOMAXCONN);

            if (err == SOCKET_ERROR) {
                Close();
                return;
            }
        }
    }

    bool IsOpen() const override {
        lock.lock();
        auto is_open = handle != INVALID_SOCKET;
        lock.unlock();
        return is_open;
    }

    virtual bool IsServer() const override = 0;

    void Send(const void* data, size_t size) const override {
        if (!IsOpen()) {
            std::cerr << "Cannot send data with a closed socket" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        auto buffer = reinterpret_cast<const char*>(data);

        int err = 0;
        size_t sent = 0;

        while (sent != size) {
            lock.lock();
            err = send(handle, &buffer[sent], size - sent, 0);
            lock.unlock();

            if (err == SOCKET_ERROR) break;

            sent += err;
        }

        if (err == SOCKET_ERROR) {
            Close();
        }
    }

    size_t Recv(void* buffer, size_t size) const override {
        if (!IsOpen()) {
            std::cerr << "Cannot recv data with a closed socket" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        lock.lock();
        int err = recv(handle, reinterpret_cast<char*>(buffer), size, 0);
        lock.unlock();

        if (err == SOCKET_ERROR) {
            Close();
            return 0;
        }

        return err;
    }

    virtual std::shared_ptr<TCPSocket> Accept() const override = 0;
};

class WindowsClientTCPSocket : public WindowsTCPSocket {
protected:
    int Bind(const std::string& address, uint16_t port, uint16_t family) const override {
        sockaddr_in addr;
        addr.sin_family = family;
        addr.sin_addr.s_addr = inet_addr(address.c_str());
        addr.sin_port = port;

        return connect(handle, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
    }

public:
    WindowsClientTCPSocket(const std::string& address, uint16_t port) {
        Setup(address, port);
    }

    WindowsClientTCPSocket(SOCKET handle) {
        this->handle = handle;
    }

    bool IsServer() const override {
        return false;
    }

    std::shared_ptr<TCPSocket> Accept() const override {
        if (!IsServer()) {
            std::cerr << "Cannot accept a new connection with a client socket" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        return nullptr;
    }
};

class WindowsServerTCPSocket : public WindowsTCPSocket {
protected:
    int Bind(const std::string&, uint16_t port, uint16_t family) const override {
        sockaddr_in addr;
        addr.sin_family = family;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = port;

        return bind(handle, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
    }

public:
    WindowsServerTCPSocket(uint16_t port) {
        Setup("", port);
    }

    bool IsServer() const override {
        return true;
    }

    std::shared_ptr<TCPSocket> Accept() const override {
        if (!IsOpen()) {
            std::cerr << "Cannot accept a new connection with a closed socket" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        SOCKET client = accept(handle, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            Close();
            return nullptr;
        }

        return std::shared_ptr<TCPSocket>(new WindowsClientTCPSocket(client));
    }
};

class WindowsUDPSocket : public UDPSocket, public WindowsWSAUser {
protected:
    mutable std::unordered_map<ClientID, sockaddr_in> clients;

    sockaddr_in server_addr;

    mutable SOCKET handle = INVALID_SOCKET;

    virtual int Bind(const std::string& address, uint16_t port, uint16_t family) = 0;

    virtual bool IsServer() const = 0;

    void Close() const {
        lock.lock();
        if (handle != INVALID_SOCKET) {
            closesocket(handle);
            handle = INVALID_SOCKET;
        }
        lock.unlock();
    }

public:
    WindowsUDPSocket(uint16_t port) : UDPSocket(port), WindowsWSAUser() {}
    
    virtual ~WindowsUDPSocket() {
        Close();
    }

    void Setup(const std::string& address, uint16_t port) {
        uint16_t family;
        if (address.find(":") == std::string::npos) {
            family = AF_INET;
        } else {
            family = AF_INET6;
        }

        handle = socket(family, SOCK_DGRAM, 0);

        if (handle == INVALID_SOCKET) {
            return;
        }

        int err = Bind(address, port, family);

        if (err == SOCKET_ERROR) {
            Close();
            return;
        }
    }

    bool IsOpen() const override {
        lock.lock();
        auto is_open = handle != INVALID_SOCKET;
        lock.unlock();
        return is_open;
    }

    void Send(const void* data, size_t size, ClientID to) const override {
        if (!IsOpen()) {
            std::cerr << "Cannot send data with a closed socket" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        auto buffer = reinterpret_cast<const char*>(data);

        int err = 0;
        size_t sent = 0;

        sockaddr_in addr;
        
        if (clients.find(to) != clients.end()) {
            addr = clients[to];
        } else if (!IsServer()) {
            addr = this->server_addr;
        } else {
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = port;
        }

        while (sent != size) {
            lock.lock();
            err = sendto(handle, &buffer[sent], size - sent, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
            lock.unlock();

            if (err == SOCKET_ERROR) break;

            sent += err;
        }

        if (err == SOCKET_ERROR) {
            err = WSAGetLastError();


            #define ERR(e) case e: std::cout << #e << std::endl; break;
            switch (err) {
                ERR(WSANOTINITIALISED)
                ERR(WSAENETDOWN)
                ERR(WSAEACCES)
                ERR(WSAEINVAL)
                ERR(WSAEINTR)
                ERR(WSAEINPROGRESS)
                ERR(WSAEFAULT)
                ERR(WSAENETRESET)
                ERR(WSAENOBUFS)
                ERR(WSAENOTCONN)
                ERR(WSAENOTSOCK)
                ERR(WSAEOPNOTSUPP)
                ERR(WSAESHUTDOWN)
                ERR(WSAEWOULDBLOCK)
                ERR(WSAEMSGSIZE)
                ERR(WSAEHOSTUNREACH)
                ERR(WSAECONNABORTED)
                ERR(WSAECONNRESET)
                ERR(WSAEADDRNOTAVAIL)
                ERR(WSAEAFNOSUPPORT)
                ERR(WSAEDESTADDRREQ)
                ERR(WSAENETUNREACH)
                ERR(WSAETIMEDOUT)
                default:
                    std::cout << "Unknown" << std::endl;
            }

            #undef ERR
            Close();
        }
    }

    size_t Recv(void* buffer, size_t size, ClientID& from) const override {
        if (!IsOpen()) {
            std::cerr << "Cannot recv data with a closed socket" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        sockaddr_in addr;
        int addr_size = sizeof(sockaddr);

        lock.lock();
        int err = recvfrom(handle, reinterpret_cast<char*>(buffer), size, 0, reinterpret_cast<sockaddr*>(&addr), &addr_size);
        lock.unlock();

        if (err == SOCKET_ERROR) {
            Close();
            from = 0;
            return 0;
        }

        lock.lock();
        for (auto& client : clients) {
            if (client.second.sin_addr.s_addr == addr.sin_addr.s_addr) {
                from = client.first;
                lock.unlock();

                return err;
            }
        }
        clients[next_free_id] = addr;
        from = next_free_id;
        next_free_id++;
        lock.unlock();

        return err;
    }
};

class WindowsClientUDPSocket : public WindowsUDPSocket {
protected:
    int Bind(const std::string& address, uint16_t port, uint16_t family) override {
        sockaddr_in addr;
        addr.sin_family = family;
        addr.sin_addr.s_addr = inet_addr(address.c_str());
        addr.sin_port = port;

        server_addr = addr;

        return connect(handle, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
    }

    bool IsServer() const override {
        return false;
    }

public:
    WindowsClientUDPSocket(const std::string& address, uint16_t port) : WindowsUDPSocket(port) {
        Setup(address, port);
    }
};

class WindowsServerUDPSocket : public WindowsUDPSocket {
protected:
    int Bind(const std::string&, uint16_t port, uint16_t family) override {
        sockaddr_in addr;
        addr.sin_family = family;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = port;

        return bind(handle, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
    }

    bool IsServer() const override {
        return true;
    }

public:
    WindowsServerUDPSocket(uint16_t port) : WindowsUDPSocket(port) {
        Setup("", port);
    }
};

#endif

std::shared_ptr<TCPSocket> TCPSocket::CreateServer(uint16_t port) {
#if defined(_WIN32) || defined(_WIN64)
    return std::shared_ptr<TCPSocket>(new WindowsServerTCPSocket(port));
#endif
}

std::shared_ptr<TCPSocket> TCPSocket::CreateClient(const std::string& address, uint16_t port) {
#if defined(_WIN32) || defined(_WIN64)
    return std::shared_ptr<TCPSocket>(new WindowsClientTCPSocket(address == "localhost" ? "127.0.0.1" : address, port));
#endif
}

std::shared_ptr<UDPSocket> UDPSocket::CreateServer(uint16_t port) {
#if defined(_WIN32) || defined(_WIN64)
    return std::shared_ptr<UDPSocket>(new WindowsServerUDPSocket(port));
#endif
}

std::shared_ptr<UDPSocket> UDPSocket::CreateClient(const std::string& address, uint16_t port) {
#if defined(_WIN32) || defined(_WIN64)
    return std::shared_ptr<UDPSocket>(new WindowsClientUDPSocket(address == "localhost" ? "127.0.0.1" : address, port));
#endif
}