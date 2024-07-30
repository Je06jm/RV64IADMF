#ifndef RV64_HPP
#define RV64_HPP

#include <cstdint>
#include <string>
#include <array>

#include "Types.hpp"

union RVInstructionWord {
    struct {
        Word _unused : 7;
        Word rd : 5;
        Word funct3 : 3;
        Word rs1 : 5;
        Word rs2 : 5;
        Word funct7 : 7;
    } R;

    struct {
        Word _unused : 7;
        Word rd : 5;
        Word funct3 : 3;
        Word rs1 : 5;
        Word rs2 : 5;
        Word funct2 : 2;
        Word rs3 : 5;
    } R4;

    struct {
        Word _unused : 7;
        Word rd : 5;
        Word funct3 : 3;
        Word rs1 : 5;
        Word imm : 12;
    } I;

    struct {
        Word _unused : 7;
        Word imm0 : 5;
        Word funct3 : 3;
        Word rs1 : 5;
        Word rs2 : 5;
        Word imm1 : 7;
    } S;

    struct {
        Word _unused : 7;
        Word imm2 : 1;
        Word imm0 : 4;
        Word funct3 : 3;
        Word rs1 : 5;
        Word rs2 : 5;
        Word imm1 : 6;
        Word imm3 : 1;
    } B;

    struct {
        Word _unused : 7;
        Word rd : 5;
        Word imm : 20;
    } U;

    struct {
        Word _unused : 7;
        Word rd : 5;
        Word imm2 : 8;
        Word imm1 : 1;
        Word imm0 : 10;
        Word imm3 : 1;
    } J;

    struct {
        Word opcode : 7;
        Word _unused : 25;
    };

    Word raw;
};

struct RVInstruction {
    enum class Type {
        LUI,
        AUIPC,
        JAL,
        JALR,
        BEQ,
        BNE,
        BLT,
        BGE,
        BLTU,
        BGEU,
        LB,
        LH,
        LW,
        LBU,
        LHU,
        SB,
        SH,
        SW,
        ADDI,
        SLTI,
        SLTIU,
        XORI,
        ORI,
        ANDI,
        SLLI,
        SRLI,
        SRAI,
        ADD,
        SUB,
        SLL,
        SLT,
        SLTU,
        XOR,
        SRL,
        SRA,
        OR,
        AND,
        FENCE,
        ECALL,
        EBREAK,
        LWU,
        LD,
        SD,
        ADDIW,
        SLLIW,
        SRLIW,
        SRAIW,
        ADDW,
        SUBW,
        SLLW,
        SRLW,
        SRAW,
        CSRRW,
        CSRRS,
        CSRRC,
        CSRRWI,
        CSRRSI,
        CSRRCI,
        MUL,
        MULH,
        MULHSU,
        MULHU,
        DIV,
        DIVU,
        REM,
        REMU,
        MULW,
        DIVW,
        DIVUW,
        REMW,
        REMUW,
        LR_W,
        SC_W,
        AMOSWAP_W,
        AMOADD_W,
        AMOXOR_W,
        AMOAND_W,
        AMOOR_W,
        AMOMIN_W,
        AMOMAX_W,
        AMOMINU_W,
        AMOMAXU_W,
        LR_D,
        SC_D,
        AMOSWAP_D,
        AMOADD_D,
        AMOXOR_D,
        AMOAND_D,
        AMOOR_D,
        AMOMIN_D,
        AMOMAX_D,
        AMOMINU_D,
        AMOMAXU_D,
        FLW,
        FSW,
        FMADD_S,
        FMSUB_S,
        FNMSUB_S,
        FNMADD_S,
        FADD_S,
        FSUB_S,
        FMUL_S,
        FDIV_S,
        FSQRT_S,
        FSGNJ_S,
        FSGNJN_S,
        FSGNJX_S,
        FMIN_S,
        FMAX_S,
        FCVT_W_S,
        FCVT_WU_S,
        FMV_X_W,
        FEQ_S,
        FLT_S,
        FLE_S,
        FCLASS_S,
        FCVT_S_W,
        FCVT_S_WU,
        FMV_W_X,
        FCVT_L_S,
        FCVT_LU_S,
        FCVT_S_L,
        FCVT_S_LU,
        FLD,
        FSD,
        FMADD_D,
        FMSUB_D,
        FNMSUB_D,
        FNMADD_D,
        FADD_D,
        FSUB_D,
        FMUL_D,
        FDIV_D,
        FSQRT_D,
        FSGNJ_D,
        FSGNJN_D,
        FSGNJX_D,
        FMIN_D,
        FMAX_D,
        FCVT_S_D,
        FCVT_D_S,
        FEQ_D,
        FLT_D,
        FLE_D,
        FCLASS_D,
        FCVT_W_D,
        FCVT_WU_D,
        FCVT_D_W,
        FCVT_D_WU,
        FCVT_L_D,
        FCVT_LU_D,
        FMV_X_D,
        FCVT_D_L,
        FCVT_D_LU,
        FMV_D_X,
        SRET,
        MRET,
        WFI,
        SFENCE_VMA,
        SINVAL_VMA,
        SINVAL_GVMA,
        SFENCE_W_INVAL,
        SFENCE_INVAL_IR,
        INVALID,
        CUST_TVA,
        CUST_MTRAP,
        CUST_STRAP
    };

    static constexpr Byte OP_LUI = 0b0110111;
    static constexpr Byte OP_AUIPC = 0b0010111;
    static constexpr Byte OP_JAL = 0b1101111;

    static constexpr Byte OP_JALR = 0b1100111;
    static constexpr Byte FUNCT3_JALR = 0b000;

    static constexpr Byte OP_BRANCH = 0b1100011;
    static constexpr Byte FUNCT3_BEQ = 0b000;
    static constexpr Byte FUNCT3_BNE = 0b001;
    static constexpr Byte FUNCT3_BLT = 0b100;
    static constexpr Byte FUNCT3_BGE = 0b101;
    static constexpr Byte FUNCT3_BLTU = 0b110;
    static constexpr Byte FUNCT3_BGEU = 0b111;

    static constexpr Byte OP_LOAD = 0b00000011;
    static constexpr Byte FUNCT3_LB = 0b000;
    static constexpr Byte FUNCT3_LH = 0b001;
    static constexpr Byte FUNCT3_LW = 0b010;
    static constexpr Byte FUNCT3_LD = 0b011;
    static constexpr Byte FUNCT3_LBU = 0b100;
    static constexpr Byte FUNCT3_LHU = 0b101;
    static constexpr Byte FUNCT3_LWU = 0b110;

    static constexpr Byte OP_STORE = 0b0100011;
    static constexpr Byte FUNCT3_SB = 0b000;
    static constexpr Byte FUNCT3_SH = 0b001;
    static constexpr Byte FUNCT3_SW = 0b010;
    static constexpr Byte FUNCT3_SD = 0b011;

    static constexpr Byte OP_MATH_IMMEDIATE = 0b0010011;
    static constexpr Byte FUNCT3_ADDI = 0b000;
    static constexpr Byte FUNCT3_SLTI = 0b010;
    static constexpr Byte FUNCT3_SLTIU = 0b011;
    static constexpr Byte FUNCT3_XORI = 0b100;
    static constexpr Byte FUNCT3_ORI = 0b110;
    static constexpr Byte FUNCT3_ANDI = 0b111;

    static constexpr Byte OP_MATH_W_IMMEDIATE = 0b0011011;

    static constexpr Byte FUNCT3_SLLI = 0b001;
    static constexpr Byte FUNCT7_SLLI = 0b0;

    static constexpr Byte FUNCT3_SHIFT_RIGHT_IMMEDIATE = 0b101;
    static constexpr Byte FUNCT7_SRLI = 0b0;
    static constexpr Byte FUNCT7_SRAI = 0b010000;

    static constexpr Byte OP_MATH = 0b0110011;
    static constexpr Byte OP_MATH_W = 0b0111011;

    static constexpr Byte FUNCT3_ADD_SUB_MUL = 0b000;
    static constexpr Byte FUNCT7_ADD = 0b0;
    static constexpr Byte FUNCT7_SUB = 0b0100000;
    static constexpr Byte FUNCT7_MUL = 0b0000001;

    static constexpr Byte FUNCT3_SLL_MULH = 0b001;
    static constexpr Byte FUNCT7_SLL = 0b0;
    static constexpr Byte FUNCT7_MULH = 0b0000001;

    static constexpr Byte FUNCT3_SLT_MULHSU = 0b010;
    static constexpr Byte FUNCT7_SLT = 0b0;
    static constexpr Byte FUNCT7_MULHSU = 0b0000001;

    static constexpr Byte FUNCT3_SLTU_MULHU = 0b011;
    static constexpr Byte FUNCT7_SLTU = 0b0;
    static constexpr Byte FUNCT7_MULHU = 0b0000001;

    static constexpr Byte FUNCT3_XOR_DIV = 0b100;
    static constexpr Byte FUNCT7_XOR = 0b0;
    static constexpr Byte FUNCT7_DIV = 0b0000001;

    static constexpr Byte FUNCT3_SHIFT_RIGHT_DIVU = 0b101;
    static constexpr Byte FUNCT7_SRL = 0b0;
    static constexpr Byte FUNCT7_SRA = 0b0100000;
    static constexpr Byte FUNCT7_DIVU = 0b0000001;
    
    static constexpr Byte FUNCT3_OR_REM = 0b110;
    static constexpr Byte FUNCT7_OR = 0b0;
    static constexpr Byte FUNCT7_REM = 0b0000001;

    static constexpr Byte FUNCT3_AND_REMU = 0b111;
    static constexpr Byte FUNCT7_AND = 0b0;
    static constexpr Byte FUNCT7_REMU = 0b0000001;

    static constexpr Byte OP_ATOMIC = 0b0101111;
    static constexpr Byte FUNCT3_ATOMIC = 0b010;
    static constexpr Byte FUNCT3_ATOMIC_W = 0b011;
    static constexpr Byte FUNCT7_ATOMIC_MASK = 0b1111100;

    static constexpr Byte FUNCT7_LR_W = 0b0001000;
    static constexpr Byte RS2_LR_W = 0b00000;

    static constexpr Byte FUNCT7_SC_W = 0b0001100;
    static constexpr Byte FUNCT7_AMOSWAP_W = 0b0000100;
    static constexpr Byte FUNCT7_AMOADD_W = 0b0000000;
    static constexpr Byte FUNCT7_AMOXOR_W = 0b0010000;
    static constexpr Byte FUNCT7_AMOAND_W = 0b0110000;
    static constexpr Byte FUNCT7_AMOOR_W = 0b0100000;
    static constexpr Byte FUNCT7_AMOMIN_W = 0b1000000;
    static constexpr Byte FUNCT7_AMOMAX_W = 0b1010000;
    static constexpr Byte FUNCT7_AMOMINU_W = 0b1100000;
    static constexpr Byte FUNCT7_AMOMAXU_W = 0b1110000;

    static constexpr Byte OP_FL = 0b0000111;
    static constexpr Byte FUNCT3_FLW = 0b010;
    static constexpr Byte FUNCT3_FLD = 0b011;
    
    static constexpr Byte OP_FS = 0b0100111;
    static constexpr Byte FUNCT3_FSW = 0b010;
    static constexpr Byte FUNCT3_FSD = 0b011;

    static constexpr Byte FUNCT7_FUSED_MASK = 0b11;

    static constexpr Byte OP_FMADD = 0b1000011;
    static constexpr Byte OP_FMSUB = 0b1000111;
    static constexpr Byte OP_FNMSUB = 0b1001011;
    static constexpr Byte OP_FNMADD = 0b1001111;

    static constexpr Byte FUNCT2_S = 0b00;
    static constexpr Byte FUNCT2_D = 0b01;

    static constexpr Byte RM_ROUND_TO_NEAREST_TIES_EVEN = 0b000;
    static constexpr Byte RM_ROUND_TO_ZERO = 0b001;
    static constexpr Byte RM_ROUND_DOWN = 0b010;
    static constexpr Byte RM_ROUND_UP = 0b011;
    static constexpr Byte RM_ROUND_TO_NEAREST_TIES_MAX_MAGNITUDE = 0b100;
    static constexpr Byte RM_INVALID0 = 0b101;
    static constexpr Byte RM_INVALID1 = 0b110;
    static constexpr Byte RM_DYNAMIC = 0b111;

    static constexpr Byte OP_FLOAT = 0b1010011;
    static constexpr Byte FLOAT_FUNCT2_MASK = 0b11;
    static constexpr Byte FUNCT5_FADD = 0b00000;
    static constexpr Byte FUNCT5_FSUB = 0b00001;
    static constexpr Byte FUNCT5_FMUL = 0b00010;
    static constexpr Byte FUNCT5_FDIV = 0b00011;

    static constexpr Byte FUNCT5_FSQRT = 0b01011;
    static constexpr Byte RS2_FSQRT = 0;
    
    static constexpr Byte FUNCT5_FSGNJ = 0b00100;
    static constexpr Byte FUNCT3_FSGNJ = 0b000;
    static constexpr Byte FUNCT3_FSGNJN = 0b001;
    static constexpr Byte FUNCT3_FSGNJX = 0b010;

    static constexpr Byte FUNCT5_FMIN_FMAX = 0b00101;
    static constexpr Byte FUNCT3_FMIN = 0b000;
    static constexpr Byte FUNCT3_FMAX = 0b001;

    static constexpr Byte FUNCT5_FCVT_W = 0b11000;

    static constexpr Byte FUNCT5_FCLASS_FMV_X_W = 0b11100;

    static constexpr Byte FUNCT3_FCLASS = 0b001;
    static constexpr Byte RS2_FCLASS = 0b00000;

    static constexpr Byte FUNCT5_FCVT = 0b11010;
    static constexpr Byte RS2_FCVT_W = 0b00000;
    static constexpr Byte RS2_FCVT_WU = 0b00001;
    static constexpr Byte RS2_FCVT_L = 0b0010;
    static constexpr Byte RS2_FCVT_LU = 0b0011;

    static constexpr Byte FUNCT3_FMV_X_W = 0b000;
    static constexpr Byte RS2_FMV_X_W = 0b00000;

    static constexpr Byte FUNCT5_FCOMPARE = 0b10100;
    static constexpr Byte FUNCT3_FEQ = 0b010;
    static constexpr Byte FUNCT3_FLT = 0b001;
    static constexpr Byte FUNCT3_FLE = 0b000;

    static constexpr Byte FUNCT5_FMV_W_X = 0b11110;
    static constexpr Byte FUNCT3_FMV_W_X = 0b000;
    static constexpr Byte RS2_FMV_W_X = 0b00000;

    static constexpr Byte FUNCT5_FCVT_D = 0b01000;

    static constexpr Byte RS2_FCVT_S_D = 0b00001;
    static constexpr Byte RS2_FCVT_D_S = 0b00000;

    static constexpr Byte OP_FENCE = 0b0001111;
    static constexpr Byte FUNCT3_FENCE = 0b000;

    static constexpr Byte OP_SYSTEM = 0b1110011;
    static constexpr Byte FUNCT3_SYSTEM = 0b000;
    static constexpr Byte RS1_SYSTEM = 0b00000;
    static constexpr Byte RD_SYSTEM = 0b00000;
    static constexpr Half IMM_ECALL = 0b0;
    static constexpr Half IMM_EBREAK = 0b000000000001;

    static constexpr Half IMM_MRET = 0b001100000010;
    static constexpr Half IMM_SRET = 0b000100000010;
    static constexpr Byte RS2_SRET_MRET = 0b00010;

    static constexpr Half IMM_WFI = 0b000100000101;
    static constexpr Byte RS2_WFI = 0b00101;
    
    static constexpr Byte FUNCT7_SFENCE_VMA = 0b0001001;
    static constexpr Byte FUNCT7_SINVAL_VMA = 0b0001011;
    static constexpr Byte FUNCT7_SINVAL_GVMA = 0b1001011;
    
    static constexpr Byte FUNCT7_SFENCE_SINVAL = 0b0001100;
    static constexpr Byte RS2_SFENCE_W_INVAL = 0b00000;
    static constexpr Byte RS2_SFENCE_INVAL_IR = 0b00001;

    static constexpr Byte OP_CSR = 0b1110011;
    static constexpr Byte FUNCT3_CSRRW = 0b001;
    static constexpr Byte FUNCT3_CSRRS = 0b010;
    static constexpr Byte FUNCT3_CSRRC = 0b011;
    static constexpr Byte FUNCT3_CSRRWI = 0b101;
    static constexpr Byte FUNCT3_CSRRSI = 0b110;
    static constexpr Byte FUNCT3_CSRRCI = 0b111;

    static constexpr Byte OP_CUST = 0b1000000;
    static constexpr Byte FUNCT7_CUST_TVA = 0b0000000;
    static constexpr Byte FUNCT7_CUST_MTRAP = 0b0000001;
    static constexpr Byte FUNCT7_CUST_STRAP = 0b0000010;

    static std::array<std::string, 32> register_names;
    static std::array<std::string, 32> fregister_names;
    static std::array<std::string, 4096> csr_names;
    static void SetupCSRNames();

    Type type = Type::INVALID;

    union {
        Long immediate;
        SLong s_immediate;
    };
    Byte rd, rs1, rs2;

    Byte rm;
    Byte rs3;

    operator std::string();

    static RVInstruction FromUInt32(Word instr);
};

#endif