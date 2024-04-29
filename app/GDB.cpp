#include "GDB.hpp"

#include <iostream>
#include <cstdlib>
#include <format>

bool GDBServer::CheckPacket(const std::string& packet) const {
    uint8_t checksum = 0;
    bool has_checksum = false;
    for (size_t i = 1; i < packet.size(); i++) {
        if (packet[i] == '#') {
            has_checksum = true;
            break;
        }
        checksum += packet[i];
    }

    if (!has_checksum) return false;

    uint8_t packet_checksum = std::stoul(packet.substr(packet.size() - 2));
    return checksum == packet_checksum;
}

std::vector<std::string> GDBServer::RecvPacket() const {
    constexpr size_t BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    std::string str_buffer;

    while (true) {
        auto size = gdb_client->Recv(buffer, BUFFER_SIZE);
        str_buffer.resize(size);
        for (size_t i = 0; i < size; i++)
            str_buffer[i] = buffer[i];
        
        if (CheckPacket(str_buffer)) {
            SendPacket("-");
            break;
        }
    }

    SendPacket("+");

    std::vector<std::string> packet;
    str_buffer = str_buffer.substr(1, str_buffer.size() - 4);

    static std::vector delimiters = {',', ';', ':'};
    while (true) {
        size_t min_index = std::string::npos;

        for (auto delimiter : delimiters) {
            size_t index = str_buffer.find(delimiter);
            if (index < min_index)
                min_index = index;
        }

        if (min_index == std::string::npos)
            break;
        
        auto first = str_buffer.substr(0, min_index);
        str_buffer = str_buffer.substr(min_index + 1);

        packet.emplace_back(std::move(first));
    }

    if (str_buffer.size() != 0)
        packet.emplace_back(std::move(str_buffer));

    for (auto& field : packet) {
        auto index = field.find('*');
        if (index == std::string::npos)
            continue;
        
        auto repeat = field[index + 1] - 29;
        std::string repeated;
        repeated.resize(repeat, field[index]);
        
        field = field.substr(0, index) + repeated + field.substr(index + 1);
    }

    return packet;
}

void GDBServer::SendPacket(const std::string& packet) const {
    uint8_t checksum = 0;
    for (auto c : packet)
        checksum += c;
    
    std::string packaged = "$" + packet + '#' + std::format("{:0>x}", checksum);

    gdb_client->Send(packaged.data(), packaged.size());
}

void GDBServer::Run(uint16_t port) {
    using VM = VirtualMachine;

    socket = TCPSocket::CreateServer(port);
    
    if (!socket->IsOpen()) {
        std::cerr << std::format("Cannot open GDB server port {}", port) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    gdb_client = socket->Accept();
    uint32_t current_hart = 0;

    vms[current_hart]->Pause();

    while (running) {
        auto packet = RecvPacket();

        auto vm = vms[current_hart];

        if (packet[0] == "D") {
            vm->Unpause();
            break;
        }
        else {
            switch (packet[0][0]) {
                case 'c': {
                    if (packet[0].size() > 1) {
                        auto s_addr = packet[0].substr(1);
                        auto addr = std::stoull(s_addr) ;

                        vm->SetPC(static_cast<uint32_t>(addr));
                    }

                    vm->Unpause();
                    break;
                }
                
                case 'g': {
                    std::array<uint32_t, VM::REGISTER_COUNT> regs;
                    std::array<Float, VM::REGISTER_COUNT> fregs;
                    uint32_t pc;

                    vm->GetSnapshot(regs, fregs, pc);

                    std::string packet;
                    for (auto reg : regs)
                        packet += std::format("{:0>8x}", reg);
                    
                    for (auto freg : fregs) {
                        if (freg.is_double)
                            packet += std::format("{:0>16x}", freg.u64);
                        
                        else
                            packet += std::format("{:0>16x}", freg.u32);
                    }

                    SendPacket(packet);
                    break;
                }

                case 'H': {
                    uint32_t thread = std::stoull(packet[0].substr(1));
                    if (thread >= vms.size())
                        SendPacket("E");
                    
                    else {
                        vm->Unpause();
                        current_hart = thread;
                        vms[current_hart]->Pause();
                    }
                    
                    break;
                }

                case 'm': {
                    auto addr = std::stoull(packet[0].substr(1));
                    auto len = std::stoull(packet[1]);

                    len = (len + 3) / 4;

                    auto words = memory.PeekWords(static_cast<uint32_t>(addr), static_cast<uint32_t>(len));

                    std::string packet;

                    for (auto word : words) {
                        if (!word.second) break;
                        packet += std::format("{:0>2x}{:0>2x}{:0>2x}{:0>2x}", word.first & 0xff, (word.first >> 8) & 0xff, (word.first >> 16) & 0xff, (word.first >> 24) & 0xff);
                    }

                    SendPacket(packet);
                    break;
                }

                case 's': {
                    if (packet[0].size() > 1) {
                        auto addr = std::stoull(packet[0].substr(1));
                        vm->SetPC(addr);
                    }

                    vm->Step(1);

                    break;
                }

                case 'T': {
                    auto thread = std::stoull(packet[0].substr(1));

                    if (thread >= vms.size())
                        SendPacket("E");
                    
                    else
                        SendPacket("OK");
                    
                    break;
                }

                case 'z': {
                    auto type = packet[0][1];
                    
                    if (type != '1') {
                        SendPacket("`'");
                        break;
                    }

                    vm->ClearBreakPoint(static_cast<uint32_t>(std::stoull(packet[1])));
                    SendPacket("OK");
                    break;
                }
                case 'Z': {
                    auto type = packet[0][1];
                    
                    if (type != '1') {
                        SendPacket("`'");
                        break;
                    }

                    vm->SetBreakPoint(static_cast<uint32_t>(std::stoull(packet[1])));
                    SendPacket("OK");
                    break;
                }
                default:
                    SendUnsupported();
                    break;
            }
        }
    }
}