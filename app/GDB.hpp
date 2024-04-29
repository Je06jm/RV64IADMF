#ifndef APP_GDB_HPP
#define APP_GDB_HPP

#include "VirtualMachines.hpp"
#include "Socket.hpp"

#include <Memory.hpp>

#include <memory>
#include <string>
#include <vector>

class GDBServer {
private:
    Memory& memory;
    
    mutable std::shared_ptr<TCPSocket> socket = nullptr;
    mutable std::shared_ptr<TCPSocket> gdb_client = nullptr;

    bool CheckPacket(const std::string& packet) const;

    std::vector<std::string> RecvPacket() const;
    void SendPacket(const std::string& packet) const;

    inline void SendUnsupported() const {
        SendPacket("#00");
    }

    bool running = true;

public:
    GDBServer(Memory& memory) : memory{memory} {}
    ~GDBServer() = default;

    void Run(uint16_t port);
    inline void Stop() { running = false; }
};

#endif