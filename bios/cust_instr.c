#include "cust_instr.h"

typedef union {
    struct {
        uint32_t opcode : 7;
        uint32_t rd : 5;
        uint32_t funct3 : 3;
        uint32_t rs1 : 5;
        uint32_t rs2 : 5;
        uint32_t funct7 : 7;
    };
    uint32_t raw;
} CustInstr;

void write_cust_instr(uint32_t* instr, uint8_t function, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    CustInstr* ci = (CustInstr*)instr;
    ci->raw = 0;

    ci->funct7 = function;
    ci->rs2 = rs2;
    ci->rs1 = rs1;
    ci->rd = rd;
    ci->opcode = 0b1000000;
}