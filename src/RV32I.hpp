#ifndef RV32I_HPP
#define RV32I_HPP

#include <cstdint>
#include <string>

struct RVInstruction {
    uint32_t immediate;
    uint8_t opcode;
    uint8_t rd, rs1, rs2;
    uint8_t func3, func7;

    operator std::string();

    static RVInstruction FromUInt32(uint32_t instr);
};

#endif