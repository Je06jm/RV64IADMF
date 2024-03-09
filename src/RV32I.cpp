#include "RV32I.hpp"

#include <format>

std::array<std::string, 32> RVInstruction::register_names = {
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "s0 / fp",
    "s1",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",
    "t3",
    "t4",
    "t5",
    "t6"
};

std::array<std::string, 32> RVInstruction::fregister_names = {
    "ft0",
    "ft1",
    "ft2",
    "ft3",
    "ft4",
    "ft5",
    "ft6",
    "ft7",
    "fs0",
    "fs1",
    "fa0",
    "fa1",
    "fa2",
    "fa3",
    "fa4",
    "fa5",
    "fa6",
    "fa7",
    "fs2",
    "fs3",
    "fs4",
    "fs5",
    "fs6",
    "fs7",
    "fs8",
    "fs9",
    "fs10",
    "fs11",
    "ft8",
    "ft9",
    "ft10",
    "ft11"
};

std::array<std::string, 4096> RVInstruction::csr_names = {};

void RVInstruction::SetupCSRNames() {
    for (size_t i = 0; i < csr_names.size(); i++) {
        csr_names[i] = std::format("csr{}", i);
    }

    csr_names[0x001] = "fflags";
    csr_names[0x002] = "frm";
    csr_names[0x003] = "fcsr";

    csr_names[0xc00] = "cycle";
    csr_names[0xc01] = "time";
    csr_names[0xc02] = "instret";
    
    for (size_t i = 3; i < 32; i++) {
        csr_names[0xc00 + i] = std::format("hpmcounter{}", i);
    }

    csr_names[0xc80] = "cycleh";
    csr_names[0xc81] = "timeh";
    csr_names[0xc82] = "instreth";

    for (size_t i = 3; i < 32; i++) {
        csr_names[0xc80 + i] = std::format("hpmcounterh{}", i);
    }

    csr_names[0x100] = "sstatus";
    csr_names[0x104] = "sie";
    csr_names[0x105] = "stvec";
    csr_names[0x106] = "scounteren";
    csr_names[0x10a] = "senvcfg";
    csr_names[0x140] = "sscratch";
    csr_names[0x141] = "sepc";
    csr_names[0x142] = "scause";
    csr_names[0x143] = "stval";
    csr_names[0x144] = "sip";
    csr_names[0x180] = "satp";
    csr_names[0x5a8] = "scontext";

    csr_names[0xf11] = "mvendorid";
    csr_names[0xf12] = "marchid";
    csr_names[0xf13] = "mimpid";
    csr_names[0xf14] = "mhartid";
    csr_names[0xf15] = "mconfigptr";
    csr_names[0x300] = "mstatus";
    csr_names[0x301] = "misa";
    csr_names[0x302] = "medeleg";
    csr_names[0x303] = "mideleg";
    csr_names[0x304] = "mie";
    csr_names[0x305] = "mtvec";
    csr_names[0x306] = "mcounteren";
    csr_names[0x310] = "mstatush";
    csr_names[0x340] = "mscratch";
    csr_names[0x341] = "mepc";
    csr_names[0x342] = "mcause";
    csr_names[0x343] = "mtval";
    csr_names[0x344] = "mip";
    csr_names[0x34a] = "mtinst";
    csr_names[0x34b] = "mtval2";

    for (size_t i = 0; i < 64; i++) {
        csr_names[0x3a0 + i] = std::format("pmpcfg{}", i);
    }

    csr_names[0xb00] = "mcycle";
    csr_names[0xb02] = "minstret";
    
    for (size_t i = 3; i < 32; i++) {
        csr_names[0xb00 + i] = std::format("mhpmcounter{}", i);
    }

    csr_names[0xb80] = "mcycleh";
    csr_names[0xb82] = "minstreth";
    
    for (size_t i = 3; i < 32; i++) {
        csr_names[0xb80 + i] = std::format("mhpmcounterh{}", i);
    }
    
    csr_names[0x7a0] = "tselect";
    csr_names[0x7a1] = "tdata1";
    csr_names[0x7a2] = "tdata2";
    csr_names[0x7a3] = "tdata3";
    csr_names[0x7a8] = "mcontext";
}

RVInstruction::operator std::string() {
    std::string s;

    switch (opcode) {
        case OP_LUI:
            s = std::format("LUI {}, {}", register_names[rd], immediate >> 12);
            break;
        
        case OP_AUIPC:
            s = std::format("AUIPC {}, {}", register_names[rd], immediate);
            break;
        
        case OP_JAL:
            s = std::format("JAL {}, {}", register_names[rd], immediate);
            break;
        
        case OP_JALR:
            s = std::format("JALR {}, {}, {}", register_names[rd], register_names[rs1], immediate);
            break;
        
        case OP_BRANCH:
            switch (func3) {
                case FUNC3_BEQ:
                    s = std::format("BEQ {}, {}, {}", register_names[rs1], register_names[rs2], immediate);
                    break;
                
                case FUNC3_BNE:
                    s = std::format("BNE {}, {}, {}", register_names[rs1], register_names[rs2], immediate);
                    break;
                
                case FUNC3_BLT:
                    s = std::format("BLT {}, {}, {}", register_names[rs1], register_names[rs2], immediate);
                    break;
                
                case FUNC3_BLTU:
                    s = std::format("BLTU {}, {}, {}", register_names[rs1], register_names[rs2], immediate);
                    break;
                
                case FUNC3_BGEU:
                    s = std::format("BGEU {}, {}, {}", register_names[rs1], register_names[rs2], immediate);
                    break;
                
                default:
                    s = "Invalid";
                    break;
            }
            break;
        
        case OP_LOAD:
            switch (func3) {
                case FUNC3_LB:
                    s = std::format("LB {}, {}({})", register_names[rd], immediate, register_names[rs2]);
                    break;
                
                case FUNC3_LH:
                    s = std::format("LH {}, {}({})", register_names[rd], immediate, register_names[rs2]);
                    break;
                
                case FUNC3_LW:
                    s = std::format("LW {}, {}({})", register_names[rd], immediate, register_names[rs2]);
                    break;
                
                case FUNC3_LBU:
                    s = std::format("LBU {}, {}({})", register_names[rd], immediate, register_names[rs2]);
                    break;
                
                case FUNC3_LHU:
                    s = std::format("LHU {}, {}({})", register_names[rd], immediate, register_names[rs2]);
                    break;
                
                default:
                    s = "Invalid";
                    break;
            }
            break;
        
        case OP_STORE:
            switch (func3) {
                case FUNC3_SB:
                    s = std::format("SB {}, {}({})", register_names[rs1], immediate, register_names[rs2]);
                    break;
                
                case FUNC3_SH:
                    s = std::format("SH {}, {}({})", register_names[rs1], immediate, register_names[rs2]);
                    break;
                
                case FUNC3_SW:
                    s = std::format("SW {}, {}({})", register_names[rs1], immediate, register_names[rs2]);
                    break;
                
                default:
                    s = "Invalid";
                    break;
            }
            break;

        case OP_MATH_IMMEDIATE:
            switch (func3) {
                case FUNC3_ADDI:
                    s = std::format("ADDI {}, {}, {}", register_names[rd], register_names[rs1], immediate);
                    break;
                
                case FUNC3_SLTI:
                    s = std::format("SLTI {}, {}, {}", register_names[rd], register_names[rs1], immediate);
                    break;
                
                case FUNC3_SLTIU:
                    s = std::format("SLTIU {}, {}, {}", register_names[rd], register_names[rs1], immediate);
                    break;
                
                case FUNC3_XORI:
                    s = std::format("XORI {}, {}, {}", register_names[rd], register_names[rs1], immediate);
                    break;
                
                case FUNC3_ORI:
                    s = std::format("ORI {}, {}, {}", register_names[rd], register_names[rs1], immediate);
                    break;
                
                case FUNC3_ANDI:
                    s = std::format("ANDI {}, {}, {}", register_names[rd], register_names[rs1], immediate);
                    break;
                
                case FUNC3_SLLI:
                    if (func7 == FUNC7_SLLI)
                        s = std::format("SLLI {}, {}, {}", register_names[rd], register_names[rs1], rs2);
                    
                    else
                        s = "Invalid";
                    
                    break;
                
                case FUNC3_SHIFT_RIGHT_IMMEDIATE:
                    switch (func7) {
                        case FUNC7_SRLI:
                            s = std::format("SRLI {}, {}, {}", register_names[rd], register_names[rs1], rs2);
                            break;
                        
                        case FUNC7_SRAI:
                            s = std::format("SRAI {}, {}, {}", register_names[rd], register_names[rs1], rs2);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                default:
                    s = "Invalid";
                    break;
            }
            break;
        
        case OP_MATH:
            switch (func3) {
                case FUNC3_ADD_SUB_MUL:
                    switch (func7) {
                        case FUNC7_ADD:
                            s = std::format("ADD {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_SUB:
                            s = std::format("SUB {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_MUL:
                            s = std::format("MUL {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC3_SLL_MULH:
                    switch (func7) {
                        case FUNC7_SLL:
                            s = std::format("SLL {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_MULH:
                            s = std::format("MULH {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC3_SLT_MULHSU:
                    switch (func7) {
                        case FUNC7_SLT:
                            s = std::format("SLT {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_MULHSU:
                            s = std::format("MULHSU {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC3_XOR_DIV:
                    switch (func7) {
                        case FUNC7_XOR:
                            s = std::format("XOR {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_DIV:
                            s = std::format("DIV {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC3_SHIFT_RIGHT_DIVU:
                    switch (func7) {
                        case FUNC7_SRL:
                            s = std::format("SRL {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_SRA:
                            s = std::format("SRA {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_DIVU:
                            s = std::format("DIVU {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC3_OR_REMW:
                    switch (func7) {
                        case FUNC7_OR:
                            s = std::format("OR {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_REMW:
                            s = std::format("REMW {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC3_AND_REMU:
                    switch (func7) {
                        case FUNC7_AND:
                            s = std::format("AND {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        case FUNC7_REMU:
                            s = std::format("REMU {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                default:
                    s = "Invalid";
                    break;
            }
            break;
        
        case OP_ATOMIC:
            if (func3 == FUNC3_ATOMIC) {
                switch (func7 & FUNC7_ATOMIC_MASK) {
                    case FUNC7_LR_W:
                        if (rs2 == RS2_LR_W)
                            s = std::format("LR.W {}, ({})", register_names[rd], register_names[rs1]);
                        
                        else
                            s = "Invalid";
                        break;
                    
                    case FUNC7_SC_W:
                        s = std::format("SC.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOSWAP_W:
                        s = std::format("AMOSWAP.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOADD_W:
                        s = std::format("AMOADD.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOXOR_W:
                        s = std::format("AMOXOR.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOAND_W:
                        s = std::format("AMOAND.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOOR_W:
                        s = std::format("AMOOR.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOMIN_W:
                        s = std::format("AMOMIN.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOMAX_W:
                        s = std::format("AMOMAX.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOMINU_W:
                        s = std::format("AMOMINU.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    case FUNC7_AMOMAXU_W:
                        s = std::format("AMOMAXU.W {}, {}, ({})", register_names[rd], register_names[rs2], register_names[rs1]);
                        break;
                    
                    default:
                        s = "Invalid";
                        break;
                }
            } else
                s = "Invalid";

            break;

        case OP_FLW:
            if (func3 == FUNC3_FLW)
                s = std::format("FL.W {}, {}({})", fregister_names[rd], immediate, register_names[rs1]);
            
            else
                s = "Invalid";
            
            break;
        
        case OP_FSW:
            if (func3 == FUNC3_FSW)
                s = std::format("FS.W {}, {}({})", fregister_names[rd], immediate, register_names[rs1]);
            
            else
                s = "Invalid";
            
            break;
        
        case OP_FMADD_S:
            if ((func7 & FUNC7_FUSED_MASK) == FUNC7_FMADD_S)
                s = std::format("FMADD.S {}, {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2], fregister_names[func7 >> 2]);
            
            else
                s = "Invalid";
            
            break;

        case OP_FMSUB_S:
            if ((func7 & FUNC7_FUSED_MASK) == FUNC7_FMSUB_S)
                s = std::format("FMSUB.S {}, {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2], fregister_names[func7 >> 2]);
            
            else
                s = "Invalid";
            
            break;
        
        case OP_FNMSUB_S:
            if ((func7 & FUNC7_FUSED_MASK) == FUNC7_FNMADD_S)
                s = std::format("FNMSUB.S {}, {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2], fregister_names[func7 >> 2]);
            
            else
                s = "Invalid";
            
            break;
        
        case OP_FNMADD_S:
            if ((func7 & FUNC7_FUSED_MASK) == FUNC7_FNMADD_S)
                s = std::format("FNMADD.S {}, {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2], fregister_names[func7 >> 2]);
            
            else
                s = "Invalid";
            
            break;
        
        case OP_FLOAT:
            switch (func7) {
                case FUNC7_FADD_S:
                    s = std::format("FADD.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                    break;
                
                case FUNC7_FSUB_S:
                    s = std::format("FSUB.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                    break;
                
                case FUNC7_FMUL_S:
                    s = std::format("FMUL.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                    break;
                
                case FUNC7_FDIV_S:
                    s = std::format("FDIV.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                    break;
                
                case FUNC7_FSQRT_S:
                    if (rs2 == RS2_FSQRT_S)
                        s = std::format("FSQRT.S {}, {}", fregister_names[rd], fregister_names[rs1]);
                    
                    else
                        s = "Invalid";
                    
                    break;
                
                case FUNC7_FSGNJ:
                    switch (func3) {
                        case FUNC3_FSGNJ_S:
                            s = std::format("FSGNJ.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        case FUNC3_FSGNJN_S:
                            s = std::format("FSGNJN.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        case FUNC3_FSGNJX_S:
                            s = std::format("FSGNJX.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC7_FMIN_FMAX:
                    switch (func3) {
                        case FUNC3_FMIN_S:
                            s = std::format("FMIN.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        case FUNC3_FMAX_S:
                            s = std::format("FMAX.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC7_FCVT_W:
                    switch (rs2) {
                        case RS2_FCVT_W_S:
                            s = std::format("FCVT.W.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        case RS2_FCVT_WU_S:
                            s = std::format("FCVT.WU.S {}, {}, {}", fregister_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC7_FMV_X_W_FCLASS:
                    if (rs2 == RS2_FMV_X_W_FCLASS) {
                        switch (func3) {
                            case FUNC3_FMV_X_W:
                                s = std::format("FMV.X.W {}, {}", register_names[rd], fregister_names[rs1]);
                                break;
                            
                            case FUNC3_FCLASS_S:
                                s = std::format("FCLASS {}, {}", fregister_names[rd], fregister_names[rs1]);
                                break;
                            
                            default:
                                s = "Invalid";
                                break;
                        }
                        break;
                    }
                    else
                        s = "Invalid";
                    
                    break;
                
                case FUNC7_FLOAT_EQU:
                    switch (func3) {
                        case FUNC3_FEQ_S:
                            s = std::format("FEQ.S {}, {}, {}", register_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        case FUNC3_FLT_S:
                            s = std::format("FLT.S {}, {}, {}", register_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        case FUNC3_FLE_S:
                            s = std::format("FLE.S {}, {}, {}", register_names[rd], fregister_names[rs1], fregister_names[rs2]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;
                
                case FUNC7_FCVT_S:
                    switch (rs2) {
                        case RS2_FCVT_S_W:
                            s = std::format("FCVT.S.W {}, {}", fregister_names[rd], fregister_names[rs1]);
                            break;
                        
                        case RS2_FCVT_S_WU:
                            s = std::format("FCVT.S.WU {}, {}", fregister_names[rd], fregister_names[rs1]);
                            break;
                        
                        default:
                            s = "Invalid";
                            break;
                    }
                    break;

                case FUNC7_FMV_W_X:
                    if ((rs2 == RS2_FMV_W_X) && (func3 == FUNC3_FMV_W_X))
                        s = std::format("FMV.W.X {}, {}", register_names[rd], fregister_names[rs1]);
                    
                    else
                        s = "Invalid";
                    
                    break;
                
                default:
                    s = "Invalid";
                    break;
            }
            break;

        case OP_FENCE:
            switch (immediate & 0xfff) {
                case 0b100000110011:
                    if ((rd == 0) && (func3 == 0) && (rs1) == 0)
                        s = "FENCE.TSO";
                    
                    else
                        s = std::format("FENCE");
                    
                    break;
                
                default:
                    s = "FENCE";
                    break;
            }

            if (func3 == 0b001) {
                s = "FENCE.I";
            }
            
            break;
        
        case OP_SYSTEM:
            switch (func3) {
                case FUNC3_CSRRW:
                    s = std::format("CSRRW {}, {}, {}", register_names[rd], csr_names[immediate], register_names[rs1]);
                    break;
                
                case FUNC3_CSRRS:
                    s = std::format("CSRRS {}, {}, {}", register_names[rd], csr_names[immediate], register_names[rs1]);
                    break;
                
                case FUNC3_CSRRC:
                    s = std::format("CSRRC {}, {}, {}", register_names[rd], csr_names[immediate], register_names[rs1]);
                    break;
                
                case FUNC3_CSRRWI:
                    s = std::format("CSRRWI {}, {}, {}", register_names[rd], csr_names[immediate], rs1);
                    break;
                
                case FUNC3_CSRRSI:
                    s = std::format("CSRRSI {}, {}, {}", register_names[rd], csr_names[immediate], rs1);
                    break;
                
                case FUNC3_CSRRCI:
                    s = std::format("CSRRCI {}, {}, {}", register_names[rd], csr_names[immediate], rs1);
                    break;
                
                case FUNC3_SYSTEM:
                    if ((immediate == IMM_MRET) && (rs2 == RS2_SRET_MRET) && (rs1 == RS1_SYSTEM) && (rd == RD_SYSTEM))
                        s = "SRET";
                    
                    else if ((immediate == IMM_MRET) && (rs2 == RS2_SRET_MRET) && (rs1 == RS1_SYSTEM) && (rd == RD_SYSTEM))
                        s = "MRET";
                    
                    else if ((immediate == IMM_WFI) && (rs2 == RS2_WFI) && (rs1 == RS1_SYSTEM) && (rd == RD_SYSTEM))
                        s = "WFI";
                    
                    else if ((func7 == FUNC7_SFENCE_VMA) && (rd == RD_SYSTEM))
                        s = "SFENCE.VMA";
                    
                    else if ((func7 == FUNC7_SINVAL_VMA) && (rd == RD_SYSTEM))
                        s = std::format("SINVAL.VMA {}, {}, {}", register_names[rd], register_names[rs1], register_names[rs2]);
                    
                    else if ((func7 == FUNC7_SFENCE_VMA) && (rs2 == RS2_SFENCE_W_INVAL) && (rs1 == RS1_SYSTEM) && (rd == RD_SYSTEM))
                        s = "SFENCE.W.INVAL";
                    
                    else if ((func7 == FUNC7_SFENCE_VMA) && (rs2 == RS2_SFENCE_INVAL_IR) && (rs1 == RS1_SYSTEM) && (rd == RD_SYSTEM))
                        s = "SFENCE.INVAL.IR";
                    
                    else if ((immediate == IMM_ECALL) && (rd == 0) && (rs1 == 0))
                        s = "ECALL";
                    
                    else if ((immediate == IMM_EBREAK) && (rd == 0) && (rs1 == 0))
                        s = "EBREAK";

                    else
                        s = "Invalid";

                    break;
                
                default:
                    s = "Invalid";
            }
            break;
            
        default:
            s = "Invalid";
            break;
    }

    return s;
}

RVInstruction RVInstruction::FromUInt32(uint32_t instr) {
    RVInstruction rv_inst;

    uint8_t rd = (instr >> 7) & 0x1f;
    uint8_t func3 = (instr >> 12) & 0x7;
    uint8_t rs1 = (instr >> 15) & 0x1f;
    uint8_t rs2 = (instr >> 20) & 0x1f;
    uint8_t func7 = (instr >> 25);

    rv_inst.opcode = instr & 0x7f;

    switch (instr & 0x7f) {
        case OP_LUI:
        case OP_AUIPC:
            rv_inst.rd = rd;
            rv_inst.immediate = instr & ~0xfff;
            break;
        
        case OP_JAL:
            rv_inst.rd = rd;
            rv_inst.immediate = 0;
            {
                uint32_t imm_20 = instr >> 31;
                uint32_t imm_10_1 = (instr >> 21) & 0b1111111111;
                uint32_t imm_11 = (instr >> 20) & 0b1;
                uint32_t imm_19_12 = (instr >> 12) & 0b11111111;
                rv_inst.immediate = (imm_20 << 20) | (imm_19_12 << 12) | (imm_11 << 11) | (imm_10_1 << 1);
            }
            break;
        
        case OP_JALR:
            rv_inst.rd = rd;
            rv_inst.func3 = func3;
            rv_inst.rs1 = rs1;
            rv_inst.immediate = instr >> 20;
            break;
        
        case OP_BRANCH:
            rv_inst.func3 = func3;
            rv_inst.rs1 = rs1;
            rv_inst.rs2 = rs2;
            {
                uint32_t imm_12 = instr >> 31;
                uint32_t imm_10_5 = (instr >> 25) & 0b111111;
                uint32_t imm_4_1 = (instr >> 8) & 0b1111;
                uint32_t imm_11 = (instr >> 7) & 0b1;
                rv_inst.immediate = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
            }
            break;
        
        case OP_STORE:
            rv_inst.func3 = func3;
            rv_inst.rs1 = rs1;
            rv_inst.rs2 = rs2;
            {
                uint32_t imm_4_0 = (instr >> 7) & 0b11111;
                uint32_t imm_11_5 = instr >> 25;
                rv_inst.immediate = (imm_11_5 << 5) | imm_4_0;
            }
            break;
        
        case OP_LOAD:
        case OP_MATH_IMMEDIATE:
        case OP_FENCE:
        case OP_SYSTEM:
        case OP_FLW:
            rv_inst.rd = rd;
            rv_inst.func3 = func3;
            rv_inst.rs1 = rs1;
            rv_inst.rs2 = rs2;
            rv_inst.immediate = instr >> 20;
            break;
        
        case OP_MATH:
        case OP_ATOMIC:
        case OP_FMADD_S:
        case OP_FMSUB_S:
        case OP_FNMSUB_S:
        case OP_FNMADD_S:
        case OP_FLOAT:
            rv_inst.rd = rd;
            rv_inst.func3 = func3;
            rv_inst.rs1 = rs1;
            rv_inst.rs2 = rs2;
            rv_inst.func7 = func7;
            break;
        
        case OP_FSW:
            rv_inst.func3 = func3;
            rv_inst.rs1 = rs1;
            rv_inst.rs2 = rs2;
            {
                uint32_t imm_4_0 = (instr >> 7) & 0b11111;
                uint32_t imm_11_5 = (instr >> 25) & 0b1111111;
                rv_inst.immediate = (imm_11_5 << 5) | imm_4_0;
            }
            break;
        
        default:
            rv_inst.opcode = 0;
    }

    return rv_inst;
}