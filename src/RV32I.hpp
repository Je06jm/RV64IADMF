#ifndef RV32I_HPP
#define RV32I_HPP

#include <cstdint>
#include <string>
#include <array>

struct RVInstruction {
    static constexpr uint8_t OP_LUI = 0b0110111;
    static constexpr uint8_t OP_AUIPC = 0b0010111;
    static constexpr uint8_t OP_JAL = 0b1101111;

    static constexpr uint8_t OP_JALR = 0b1100111;
    static constexpr uint8_t FUNC3_JALR = 0b000;

    static constexpr uint8_t OP_BRANCH = 0b1100011;
    static constexpr uint8_t FUNC3_BEQ = 0b000;
    static constexpr uint8_t FUNC3_BNE = 0b001;
    static constexpr uint8_t FUNC3_BLT = 0b100;
    static constexpr uint8_t FUNC3_BGE = 0b101;
    static constexpr uint8_t FUNC3_BLTU = 0b110;
    static constexpr uint8_t FUNC3_BGEU = 0b111;

    static constexpr uint8_t OP_LOAD = 0b00000011;
    static constexpr uint8_t FUNC3_LB = 0b000;
    static constexpr uint8_t FUNC3_LH = 0b001;
    static constexpr uint8_t FUNC3_LW = 0b010;
    static constexpr uint8_t FUNC3_LBU = 0b100;
    static constexpr uint8_t FUNC3_LHU = 0b101;

    static constexpr uint8_t OP_STORE = 0b0100011;
    static constexpr uint8_t FUNC3_SB = 0b000;
    static constexpr uint8_t FUNC3_SH = 0b001;
    static constexpr uint8_t FUNC3_SW = 0b010;

    static constexpr uint8_t OP_MATH_IMMEDIATE = 0b0010011;
    static constexpr uint8_t FUNC3_ADDI = 0b000;
    static constexpr uint8_t FUNC3_SLTI = 0b010;
    static constexpr uint8_t FUNC3_SLTIU = 0b011;
    static constexpr uint8_t FUNC3_XORI = 0b100;
    static constexpr uint8_t FUNC3_ORI = 0b110;
    static constexpr uint8_t FUNC3_ANDI = 0b111;

    static constexpr uint8_t FUNC3_SLLI = 0b001;
    static constexpr uint8_t FUNC7_SLLI = 0b0;

    static constexpr uint8_t FUNC3_SHIFT_RIGHT_IMMEDIATE = 0b101;
    static constexpr uint8_t FUNC7_SRLI = 0b0;
    static constexpr uint8_t FUNC7_SRAI = 0b0100000;

    static constexpr uint8_t OP_MATH = 0b0110011;

    static constexpr uint8_t FUNC3_ADD_SUB_MUL = 0b000;
    static constexpr uint8_t FUNC7_ADD = 0b0;
    static constexpr uint8_t FUNC7_SUB = 0b0100000;
    static constexpr uint8_t FUNC7_MUL = 0b0000001;

    static constexpr uint8_t FUNC3_SLL_MULH = 0b001;
    static constexpr uint8_t FUNC7_SLL = 0b0;
    static constexpr uint8_t FUNC7_MULH = 0b0000001;

    static constexpr uint8_t FUNC3_SLT_MULHSU = 0b010;
    static constexpr uint8_t FUNC7_SLT = 0b0;
    static constexpr uint8_t FUNC7_MULHSU = 0b0000001;

    static constexpr uint8_t FUNC3_SLTU_MULHU = 0b011;
    static constexpr uint8_t FUNC7_SLTU = 0b0;
    static constexpr uint8_t FUNC7_MULHU = 0b0000001;

    static constexpr uint8_t FUNC3_XOR_DIV = 0b100;
    static constexpr uint8_t FUNC7_XOR = 0b0;
    static constexpr uint8_t FUNC7_DIV = 0b0000001;

    static constexpr uint8_t FUNC3_SHIFT_RIGHT_DIVU = 0b101;
    static constexpr uint8_t FUNC7_SRL = 0b0;
    static constexpr uint8_t FUNC7_SRA = 0b0100000;
    static constexpr uint8_t FUNC7_DIVU = 0b0000001;
    
    static constexpr uint8_t FUNC3_OR_REMW = 0b110;
    static constexpr uint8_t FUNC7_OR = 0b0;
    static constexpr uint8_t FUNC7_REMW = 0b0000001;

    static constexpr uint8_t FUNC3_AND_REMU = 0b111;
    static constexpr uint8_t FUNC7_AND = 0b0;
    static constexpr uint8_t FUNC7_REMU = 0b0000001;

    static constexpr uint8_t OP_ATOMIC = 0b0101111;
    static constexpr uint8_t FUNC3_ATOMIC = 0b010;
    static constexpr uint8_t FUNC7_ATOMIC_MASK = 0b1111100;

    static constexpr uint8_t FUNC7_LR_W = 0b0001000;
    static constexpr uint8_t RS2_LR_W = 0b00000;

    static constexpr uint8_t FUNC7_SC_W = 0b0001100;
    static constexpr uint8_t FUNC7_AMOSWAP_W = 0b0000100;
    static constexpr uint8_t FUNC7_AMOADD_W = 0b0000000;
    static constexpr uint8_t FUNC7_AMOXOR_W = 0b0010000;
    static constexpr uint8_t FUNC7_AMOAND_W = 0b0110000;
    static constexpr uint8_t FUNC7_AMOOR_W = 0b0100000;
    static constexpr uint8_t FUNC7_AMOMIN_W = 0b1000000;
    static constexpr uint8_t FUNC7_AMOMAX_W = 0b1010000;
    static constexpr uint8_t FUNC7_AMOMINU_W = 0b1100000;
    static constexpr uint8_t FUNC7_AMOMAXU_W = 0b1110000;

    static constexpr uint8_t OP_FLW = 0b0000111;
    static constexpr uint8_t FUNC3_FLW = 0b010;
    
    static constexpr uint8_t OP_FSW = 0b0100111;
    static constexpr uint8_t FUNC3_FSW = 0b010;

    static constexpr uint8_t FUNC7_FUSED_MASK = 0b11;

    static constexpr uint8_t OP_FMADD_S = 0b1000011;
    static constexpr uint8_t OP_FMSUB_S = 0b1000111;
    static constexpr uint8_t OP_FNMSUB_S = 0b1001011;
    static constexpr uint8_t OP_FNMADD_S = 0b1001111;
    static constexpr uint8_t FMT_S = 0b00;

    static constexpr uint8_t RM_ROUND_TO_NEAREST_TIES_EVEN = 0b000;
    static constexpr uint8_t RM_ROUND_TO_ZERO = 0b001;
    static constexpr uint8_t RM_ROUND_DOWN = 0b010;
    static constexpr uint8_t RM_ROUND_UP = 0b011;
    static constexpr uint8_t RM_ROUND_TO_NEAREST_TIES_MAX_MAGNITUDE = 0b100;
    static constexpr uint8_t RM_INVALID0 = 0b101;
    static constexpr uint8_t RM_INVALID1 = 0b110;
    static constexpr uint8_t RM_DYNAMIC = 0b111;

    static constexpr uint8_t OP_FLOAT = 0b1010011;
    static constexpr uint8_t FUNC7_FADD_S = 0b0;
    static constexpr uint8_t FUNC7_FSUB_S = 0b0000100;
    static constexpr uint8_t FUNC7_FMUL_S = 0b0001000;
    static constexpr uint8_t FUNC7_FDIV_S = 0b0001100;

    static constexpr uint8_t FUNC7_FSQRT_S = 0b0101100;
    static constexpr uint8_t RS2_FSQRT_S = 0;
    
    static constexpr uint8_t FUNC7_FSGNJ = 0b0010000;
    static constexpr uint8_t FUNC3_FSGNJ_S = 0b000;
    static constexpr uint8_t FUNC3_FSGNJN_S = 0b001;
    static constexpr uint8_t FUNC3_FSGNJX_S = 0b010;

    static constexpr uint8_t FUNC7_FMIN_FMAX = 0b0010100;
    static constexpr uint8_t FUNC3_FMIN_S = 0b000;
    static constexpr uint8_t FUNC3_FMAX_S = 0b001;

    static constexpr uint8_t FUNC7_FCVT_W = 0b1100000;
    static constexpr uint8_t RS2_FCVT_W_S = 0b00000;
    static constexpr uint8_t RS2_FCVT_WU_S = 0b00001;

    static constexpr uint8_t FUNC7_FMV_X_W_FCLASS = 0b1110000;
    static constexpr uint8_t RS2_FMV_X_W_FCLASS = 0b00000;
    static constexpr uint8_t FUNC3_FMV_X_W = 0b000;
    static constexpr uint8_t FUNC3_FCLASS_S = 0b001;

    static constexpr uint8_t FUNC7_FLOAT_EQU = 0b1010000;
    static constexpr uint8_t FUNC3_FEQ_S = 0b010;
    static constexpr uint8_t FUNC3_FLT_S = 0b001;
    static constexpr uint8_t FUNC3_FLE_S = 0b000;

    static constexpr uint8_t FUNC7_FCVT_S = 0b1101000;
    static constexpr uint8_t RS2_FCVT_S_W = 0b00000;
    static constexpr uint8_t RS2_FCVT_S_WU = 0b00001;

    static constexpr uint8_t FUNC7_FMV_W_X = 0b1111000;
    static constexpr uint8_t RS2_FMV_W_X = 0b00000;
    static constexpr uint8_t FUNC3_FMV_W_X = 0b000;

    static constexpr uint8_t OP_FENCE = 0b0001111;
    static constexpr uint8_t FUNC3_FENCE = 0b000;

    static constexpr uint8_t OP_SYSTEM = 0b1110011;
    static constexpr uint8_t FUNC3_SYSTEM = 0b000;
    static constexpr uint8_t RS1_SYSTEM = 0b00000;
    static constexpr uint8_t RD_SYSTEM = 0b00000;
    static constexpr uint16_t IMM_ECALL = 0b0;
    static constexpr uint16_t IMM_EBREAK = 0b000000000001;

    static constexpr uint16_t IMM_URET = 0b000000000010;
    static constexpr uint16_t IMM_MRET = 0b000100000010;
    static constexpr uint16_t IMM_SRET = 0b001100000010;
    static constexpr uint8_t RS2_SRET_MRET = 0b00010;

    static constexpr uint16_t IMM_WFI = 0b000100000101;
    static constexpr uint8_t RS2_WFI = 0b00101;
    
    static constexpr uint8_t FUNC7_SFENCE_VMA = 0b0001001;
    static constexpr uint8_t FUNC7_SINVAL_VMA = 0b0001011;
    
    static constexpr uint8_t FUNC7_SFENCE_SINVAL = 0b0001100;
    static constexpr uint8_t RS2_SFENCE_W_INVAL = 0b00000;
    static constexpr uint8_t RS2_SFENCE_INVAL_IR = 0b00001;

    static constexpr uint8_t OP_CSR = 0b1110011;
    static constexpr uint8_t FUNC3_CSRRW = 0b001;
    static constexpr uint8_t FUNC3_CSRRS = 0b010;
    static constexpr uint8_t FUNC3_CSRRC = 0b011;
    static constexpr uint8_t FUNC3_CSRRWI = 0b101;
    static constexpr uint8_t FUNC3_CSRRSI = 0b110;
    static constexpr uint8_t FUNC3_CSRRCI = 0b111;

    static std::array<std::string, 32> register_names;
    static std::array<std::string, 32> fregister_names;
    static std::array<std::string, 4096> csr_names;
    static void SetupCSRNames();

    uint32_t immediate;
    uint8_t opcode;
    uint8_t rd, rs1, rs2;
    uint8_t func3;

    union {
        uint8_t func7;
        struct {
            uint8_t fmt : 2;
            uint8_t rs3 : 5;
        };
    };

    operator std::string();

    static RVInstruction FromUInt32(uint32_t instr);
};

#endif