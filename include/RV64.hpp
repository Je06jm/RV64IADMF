#ifndef RV64_HPP
#define RV64_HPP

#include <cstdint>
#include <string>
#include <array>

#include "Types.hpp"

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

    static constexpr Byte RM_ROUND_TO_NEAREST_TIES_EVEN = 0b000;
    static constexpr Byte RM_ROUND_TO_ZERO = 0b001;
    static constexpr Byte RM_ROUND_DOWN = 0b010;
    static constexpr Byte RM_ROUND_UP = 0b011;
    static constexpr Byte RM_ROUND_TO_NEAREST_TIES_MAX_MAGNITUDE = 0b100;
    static constexpr Byte RM_INVALID0 = 0b101;
    static constexpr Byte RM_INVALID1 = 0b110;
    static constexpr Byte RM_DYNAMIC = 0b111;

    static std::array<std::string, 32> register_names;
    static std::array<std::string, 32> fregister_names;
    static std::array<std::string, 4096> csr_names;
    static void SetupCSRNames();

    Type type = Type::INVALID;

    Word immediate;
    Byte rd, rs1, rs2;

    Byte rm;
    Byte rs3;

    operator std::string();

    static RVInstruction FromUInt32(Word instr);
};

#endif