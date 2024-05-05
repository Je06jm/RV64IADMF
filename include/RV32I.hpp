#ifndef RV32I_HPP
#define RV32I_HPP

#include <cstdint>
#include <string>
#include <array>

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

    static constexpr uint8_t RM_ROUND_TO_NEAREST_TIES_EVEN = 0b000;
    static constexpr uint8_t RM_ROUND_TO_ZERO = 0b001;
    static constexpr uint8_t RM_ROUND_DOWN = 0b010;
    static constexpr uint8_t RM_ROUND_UP = 0b011;
    static constexpr uint8_t RM_ROUND_TO_NEAREST_TIES_MAX_MAGNITUDE = 0b100;
    static constexpr uint8_t RM_INVALID0 = 0b101;
    static constexpr uint8_t RM_INVALID1 = 0b110;
    static constexpr uint8_t RM_DYNAMIC = 0b111;

    static std::array<std::string, 32> register_names;
    static std::array<std::string, 32> fregister_names;
    static std::array<std::string, 4096> csr_names;
    static void SetupCSRNames();

    Type type = Type::INVALID;

    uint32_t immediate;
    uint8_t rd, rs1, rs2;

    uint8_t rm;
    uint8_t rs3;

    operator std::string();

    static RVInstruction FromUInt32(uint32_t instr);
};

#endif