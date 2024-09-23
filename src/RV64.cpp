#include "RV64.hpp"

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

    union SU64 {
        Long u;
        SLong s;
    };

    SU64 imm;
    imm.u = immediate;

    auto s_rs2 = register_names[rs2];
    auto s_rs1 = register_names[rs1];
    auto s_rd = register_names[rd];

    auto s_frs3 = fregister_names[rs3];
    auto s_frs2 = fregister_names[rs2];
    auto s_frs1 = fregister_names[rs1];
    auto s_frd = fregister_names[rd];

    std::string s_imm;
    if (imm.s < 0) s_imm = std::format("{} ({})", imm.s, imm.u);
    else s_imm = std::format("{}", imm.u);

    switch (type) {
        case Type::LUI:
            s = std::format("LUI {}, {}", s_rd, s_imm);
            break;
        
        case Type::AUIPC:
            s = std::format("AUIPC {}, {}", s_rd, s_imm);
            break;
        
        case Type::JAL:
            s = std::format("JAL {}, {}", s_rd, imm.s);
            break;
        
        case Type::JALR:
            s = std::format("JALR {}, {}, {}", s_rd, s_rs1, imm.s);
            break;
        
        case Type::BEQ:
            s = std::format("BEQ {}, {}, {}", s_rs1, s_rs2, imm.s);
            break;
        
        case Type::BNE:
            s = std::format("BNE {}, {}, {}", s_rs1, s_rs2, imm.s);
            break;
        
        case Type::BLT:
            s = std::format("BLT {}, {}, {}", s_rs1, s_rs2, imm.s);
            break;
        
        case Type::BGE:
            s = std::format("BGE {}, {}, {}", s_rs1, s_rs2, imm.s);
            break;
        
        case Type::BLTU:
            s = std::format("BLTU {}, {}, {}", s_rs1, s_rs2, imm.s);
            break;
        
        case Type::BGEU:
            s = std::format("BGEU {}, {}, {}", s_rs1, s_rs2, imm.s);
            break;
        
        case Type::LB:
            s = std::format("LB {}, {}({})", s_rd, imm.s, s_rs1);
            break;
        
        case Type::LH:
            s = std::format("LH {}, {}({})", s_rd, imm.s, s_rs1);
            break;
        
        case Type::LW:
            s = std::format("LW {}, {}({})", s_rd, imm.s, s_rs1);
            break;
        
        case Type::LBU:
            s = std::format("LBU {}, {}({})", s_rd, imm.s, s_rs1);
            break;
        
        case Type::LHU:
            s = std::format("LHU {}, {}({})", s_rd, imm.s, s_rs1);
            break;
        
        case Type::SB:
            s = std::format("SB {}, {}({})", s_rs2, imm.s, s_rs1);
            break;
        
        case Type::SH:
            s = std::format("SH {}, {}({})", s_rs2, imm.s, s_rs1);
            break;
        
        case Type::SW:
            s = std::format("SW {}, {}({})", s_rs2, imm.s, s_rs1);
            break;
        
        case Type::ADDI:
            s = std::format("ADDI {}, {}, {}", s_rd, s_rs1, s_imm);
            break;
        
        case Type::SLTI:
            s = std::format("SLTI {}, {}, {}", s_rd, s_rs1, s_imm);
            break;
        
        case Type::SLTIU:
            s = std::format("SLTIU {}, {}, {}", s_rd, s_rs1, s_imm);
            break;
        
        case Type::XORI:
            s = std::format("XORI {}, {}, {}", s_rd, s_rs1, s_imm);
            break;
        
        case Type::ORI:
            s = std::format("ORI {}, {}, {}", s_rd, s_rs1, s_imm);
            break;
        
        case Type::ANDI:
            s = std::format("ANDI {}, {}, {}", s_rd, s_rs1, s_imm);
            break;
        
        case Type::SLLI:
            s = std::format("SLLI {}, {}, {}", s_rd, s_rs1, imm.u & 0b111111);
            break;
        
        case Type::SRLI:
            s = std::format("SRLI {}, {}, {}", s_rd, s_rs1, imm.u & 0b11111);
            break;
        
        case Type::SRAI:
            s = std::format("SRAI {}, {}, {}", s_rd, s_rs1, imm.u & 0b11111);
            break;
        
        case Type::ADD:
            s = std::format("ADD {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SUB:
            s = std::format("SUB {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SLL:
            s = std::format("SLL {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SLT:
            s = std::format("SLT {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SLTU:
            s = std::format("SLTU {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::XOR:
            s = std::format("XOR {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SRL:
            s = std::format("SRL {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SRA:
            s = std::format("SRA {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::OR:
            s = std::format("OR {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AND:
            s = std::format("AND {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::FENCE:
            s = std::format("FENCE");
            break;
        
        case Type::ECALL:
            s = std::format("ECALL");
            break;
        
        case Type::EBREAK:
            s = std::format("EBREAK");
            break;
        
        case Type::LWU:
            s = std::format("LWU {}, {}({})", s_rd, imm.s, s_rs1);
            break;
        
        case Type::LD:
            s = std::format("LD {}, {}({})", s_rd, imm.s, s_rs1);
            break;
        
        case Type::SD:
            s = std::format("SD {}, {}({})", s_rs2, imm.s, s_rs1);
            break;

        case Type::ADDIW:
            s = std::format("ADDI.W {}, {}, {}", s_rd, s_rs1, s_imm);
            break;
        
        case Type::SLLIW:
            s = std::format("SLLI.W {}, {}, {}", s_rd, s_rs1, s_imm);
            break;

        case Type::SUBW:
            s = std::format("SUB.W {}, {}, {}", s_rd, s_rs1, s_imm);
            break;
        
        case Type::SLLW:
            s = std::format("SLL.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SRLW:
            s = std::format("SRL.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SRAW:
            s = std::format("SRA.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;

        case Type::CSRRW:
            s = std::format("CSRRW {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::CSRRS:
            s = std::format("CSRRS {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::CSRRC:
            s = std::format("CSRRC {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::CSRRWI:
            s = std::format("CSRRWI {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::CSRRSI:
            s = std::format("CSRRSI {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::CSRRCI:
            s = std::format("CSRRCI {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::MUL:
            s = std::format("MUL {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::MULH:
            s = std::format("MULH {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::MULHSU:
            s = std::format("MULHSU {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::MULHU:
            s = std::format("MULHU {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::DIV:
            s = std::format("DIV {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::DIVU:
            s = std::format("DIVU {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::REM:
            s = std::format("REM {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::REMU:
            s = std::format("REMU {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::MULW:
            s = std::format("MUL.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::DIVW:
            s = std::format("DIV.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::DIVUW:
            s = std::format("DIVU.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::REMW:
            s = std::format("REM.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::REMUW:
            s = std::format("REMU.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::LR_W:
            s = std::format("LR.W {}, {}", s_rd, s_rs1);
            break;
        
        case Type::SC_W:
            s = std::format("SC.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOSWAP_W:
            s = std::format("AMOSWAP.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOADD_W:
            s = std::format("AMOADD.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOXOR_W:
            s = std::format("AMO XOR.W{}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOAND_W:
            s = std::format("AMOAND.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOOR_W:
            s = std::format("AMOOR.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOMIN_W:
            s = std::format("AMOMIN.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOMAX_W:
            s = std::format("AMOMAX.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOMINU_W:
            s = std::format("AMOMINU.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::AMOMAXU_W:
            s = std::format("AMOMAXU.W {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::LR_D:
            s = std::format("LR.D");
            break;
        
        case Type::FLW:
            s = std::format("FLW {}, {}, {}", s_frd, s_frs1, imm.s);
            break;
        
        case Type::FSW:
            s = std::format("FSW {}, {}, {}, {}", s_frd, s_frs1, s_frs2, imm.s);
            break;
        
        case Type::FMADD_S:
            s = std::format("FMADD.S {}, {}, {}, {}", s_frd, s_frs1, s_frs2, s_frs3);
            break;
        
        case Type::FMSUB_S:
            s = std::format("FMSUB.S {}, {}, {}, {}", s_frd, s_frs1, s_frs2, s_frs3);
            break;
        
        case Type::FNMSUB_S:
            s = std::format("FNMSUB.S {}, {}, {}, {}", s_frd, s_frs1, s_frs2, s_frs3);
            break;
        
        case Type::FNMADD_S:
            s = std::format("FNMADD.S {}, {}, {}, {}", s_frd, s_frs1, s_frs2, s_frs3);
            break;
        
        case Type::FADD_S:
            s = std::format("FADD.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FSUB_S:
            s = std::format("FSUB.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FMUL_S:
            s = std::format("FMUL.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FDIV_S:
            s = std::format("FDIV.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FSQRT_S:
            s = std::format("FSQRT.S {}, {}", s_frd, s_frs1);
            break;
        
        case Type::FSGNJ_S:
            s = std::format("FSGNJ.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FSGNJN_S:
            s = std::format("FSGNJN.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FSGNJX_S:
            s = std::format("FSGNJX.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FMIN_S:
            s = std::format("FMIN.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FMAX_S:
            s = std::format("FMAX.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;

        case Type::FCVT_W_S:
            s = std::format("FCVT.W.S {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FCVT_WU_S:
            s = std::format("FCVT.WU.S {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FMV_X_W:
            s = std::format("FMV.X.W {}, {}", s_frd, s_rs1);
            break;
        
        case Type::FEQ_S:
            s = std::format("FEQ.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FLT_S:
            s = std::format("FLT.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FLE_S:
            s = std::format("FLE.S {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FCLASS_S:
            s = std::format("FCLASS.S {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FCVT_S_W:
            s = std::format("FCVT.S.W {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FCVT_S_WU:
            s = std::format("FCVT.S.WU {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FMV_W_X:
            s = std::format("FMV.W.X {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FLD:
            s = std::format("FLD {}, {}, {}", s_frd, s_frs1, imm.s);
            break;
        
        case Type::FSD:
            s = std::format("FSD {}, {}, {}, {}", s_frd, s_frs1, s_frs2, imm.s);
            break;
        
        case Type::FMADD_D:
            s = std::format("FMADD.D {}, {}, {}, {}", s_frd, s_frs1, s_frs2, s_frs3);
            break;
        
        case Type::FMSUB_D:
            s = std::format("FMSUB.D {}, {}, {}, {}", s_frd, s_frs1, s_frs2, s_frs3);
            break;
        
        case Type::FNMSUB_D:
            s = std::format("FNMSUB.D {}, {}, {}, {}", s_frd, s_frs1, s_frs2, s_frs3);
            break;
        
        case Type::FNMADD_D:
            s = std::format("FNMADD.D {}, {}, {}, {}", s_frd, s_frs1, s_frs2, s_frs3);
            break;
        
        case Type::FADD_D:
            s = std::format("FADD.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FSUB_D:
            s = std::format("FSUB.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FMUL_D:
            s = std::format("FMUL.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FDIV_D:
            s = std::format("FDIV.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FSQRT_D:
            s = std::format("FSQRT.D {}, {}", s_frd, s_frs1);
            break;
        
        case Type::FSGNJ_D:
            s = std::format("FSGNJ.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FSGNJN_D:
            s = std::format("FSGNJN.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FSGNJX_D:
            s = std::format("FSGNJX.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FMIN_D:
            s = std::format("FMIN.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FMAX_D:
            s = std::format("FMAX.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FCVT_S_D:
            s = std::format("FCVT.S.D {}, {}", s_frd, s_frs1);
            break;
        
        case Type::FCVT_D_S:
            s = std::format("FCVT.D.S {}, {}", s_frd, s_frs1);
            break;
        
        case Type::FEQ_D:
            s = std::format("FEQ.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FLT_D:
            s = std::format("FLT.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FLE_D:
            s = std::format("FLE.D {}, {}, {}", s_frd, s_frs1, s_frs2);
            break;
        
        case Type::FCLASS_D:
            s = std::format("FCLASS.D {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FCVT_W_D:
            s = std::format("FCVT.W.D {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FCVT_WU_D:
            s = std::format("FCVT.WU.D {}, {}", s_rd, s_frs1);
            break;
        
        case Type::FCVT_D_W:
            s = std::format("FCVT.D.W {}, {}", s_frd, s_rs1);
            break;
        
        case Type::FCVT_D_WU:
            s = std::format("FCVT.D.WU {}, {}", s_frd, s_rs1);
            break;
        
        case Type::SRET:
            s = std::format("SRET");
            break;
        
        case Type::MRET:
            s = std::format("MRET");
            break;
        
        case Type::WFI:
            s = std::format("WFI");
            break;
        
        case Type::SFENCE_VMA:
            s = std::format("SFENCE.VMA {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SINVAL_VMA:
            s = std::format("SINVAL.VMA {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;

        case Type::SINVAL_GVMA:
            s = std::format("SINVAL.GVMA {}, {}, {}", s_rd, s_rs1, s_rs2);
            break;
        
        case Type::SFENCE_W_INVAL:
            s = std::format("SFENCE.W.INVAL");
            break;
        
        case Type::SFENCE_INVAL_IR:
            s = std::format("SFENCE.INVAL.IR");
            break;
        
        case Type::INVALID:
            s = std::format("INVALID");
            break;
        
        case Type::CUST_TVA:
            s = std::format("CUST.TVA {}, {}", s_rd, s_rs1);
            break;
        
        case Type::CUST_MTRAP:
            s = std::format("CUST.MTRAP {}, {}", s_rs1, s_rs2);
            break;
        
        case Type::CUST_STRAP:
            s = std::format("CUST.STRAP {}, {}", s_rs1, s_rs2);
            break;

        default:
            s = std::format("Unknown instruction type {}", int(type));
            break;
    }

    return s;
}

RVInstruction RVInstruction::FromUInt32(Word instr) {
    RVInstructionWord iw;
    iw.raw = instr;

    RVInstruction rv;
    rv.type = RVInstruction::Type::INVALID;
    rv.immediate = 0;
    rv.rd = 0;
    rv.rs1 = 0;
    rv.rs2 = 0;
    rv.rm = 0;
    rv.rs3 = 0;

    int8_t sign_extend_at = -1;

    switch (iw.opcode) {
        case OP_LUI:
            rv.type = RVInstruction::Type::LUI;
            rv.rd = iw.U.rd;
            rv.immediate = iw.U.imm << 12;
            sign_extend_at = 31;
            break;
        
        case OP_AUIPC:
            rv.type = RVInstruction::Type::AUIPC;
            rv.rd = iw.U.rd;
            rv.immediate = iw.U.imm << 12;
            sign_extend_at = 31;
            break;
        
        case OP_JAL:
            rv.type = RVInstruction::Type::JAL;
            rv.rd = iw.J.rd;
            rv.immediate = iw.J.imm0 << 1;
            rv.immediate |= iw.J.imm1 << 11;
            rv.immediate |= iw.J.imm2 << 12;
            rv.immediate |= iw.J.imm3 << 20;
            sign_extend_at = 20;
            break;
        
        case OP_JALR:
            if (iw.I.funct3 != 0) break;

            rv.type = RVInstruction::Type::JALR;
            rv.rd = iw.I.rd;
            rv.rs1 = iw.I.rs1;
            rv.immediate = iw.I.imm;
            sign_extend_at = 11;
            break;
        
        case OP_BRANCH:
            rv.rs1 = iw.B.rs1;
            rv.rs2 = iw.B.rs2;
            rv.immediate = iw.B.imm0 << 1;
            rv.immediate |= iw.B.imm1 << 5;
            rv.immediate |= iw.B.imm2 << 11;
            rv.immediate |= iw.B.imm3 << 12;
            sign_extend_at = 12;

            switch (iw.S.funct3) {
                case FUNCT3_BEQ:
                    rv.type = RVInstruction::Type::BEQ;
                    break;
                
                case FUNCT3_BNE:
                    rv.type = RVInstruction::Type::BNE;
                    break;
                
                case FUNCT3_BLT:
                    rv.type = RVInstruction::Type::BLT;
                    break;
                
                case FUNCT3_BGE:
                    rv.type = RVInstruction::Type::BGE;
                    break;
                
                case FUNCT3_BLTU:
                    rv.type = RVInstruction::Type::BLTU;
                    break;
                
                case FUNCT3_BGEU:
                    rv.type = RVInstruction::Type::BGEU;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_LOAD:
            rv.rd = iw.I.rd;
            rv.rs1 = iw.I.rs1;
            rv.immediate = iw.I.imm;
            sign_extend_at = 11;

            switch (iw.I.funct3) {
                case FUNCT3_LB:
                    rv.type = RVInstruction::Type::LB;
                    break;
                
                case FUNCT3_LH:
                    rv.type = RVInstruction::Type::LH;
                    break;
                
                case FUNCT3_LW:
                    rv.type = RVInstruction::Type::LW;
                    break;
                
                case FUNCT3_LD:
                    rv.type = RVInstruction::Type::LD;
                    break;
                
                case FUNCT3_LBU:
                    rv.type = RVInstruction::Type::LBU;
                    break;
                
                case FUNCT3_LHU:
                    rv.type = RVInstruction::Type::LHU;
                    break;
                
                case FUNCT3_LWU:
                    rv.type = RVInstruction::Type::LWU;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_STORE:
            rv.rs1 = iw.S.rs1;
            rv.rs2 = iw.S.rs2;
            rv.immediate = iw.S.imm0;
            rv.immediate |= iw.S.imm1 << 5;
            sign_extend_at = 11;

            switch (iw.S.funct3) {
                case FUNCT3_SB:
                    rv.type = RVInstruction::Type::SB;
                    break;
                
                case FUNCT3_SH:
                    rv.type = RVInstruction::Type::SH;
                    break;
                
                case FUNCT3_SW:
                    rv.type = RVInstruction::Type::SW;
                    break;
                
                case FUNCT3_SD:
                    rv.type = RVInstruction::Type::SD;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_MATH_IMMEDIATE:
            rv.rd = iw.I.rd;
            rv.rs1 = iw.I.rs1;
            rv.immediate = iw.I.imm;
            sign_extend_at = 11;
            
            switch (iw.I.funct3) {
                case FUNCT3_ADDI:
                    rv.type = RVInstruction::Type::ADDI;
                    break;
                
                case FUNCT3_SLTI:
                    rv.type = RVInstruction::Type::SLTI;
                    break;
                
                case FUNCT3_SLTIU:
                    rv.type = RVInstruction::Type::SLTIU;
                    break;
                
                case FUNCT3_XORI:
                    rv.type = RVInstruction::Type::XORI;
                    break;
                
                case FUNCT3_ORI:
                    rv.type = RVInstruction::Type::ORI;
                    break;
                
                case FUNCT3_ANDI:
                    rv.type = RVInstruction::Type::ANDI;
                    break;
                
                case FUNCT3_SLLI:
                    if ((rv.immediate >> 6) != 0) break;
                    rv.type = RVInstruction::Type::SLLI;
                    break;
                
                case FUNCT3_SHIFT_RIGHT_IMMEDIATE:
                    rv.rs2 = iw.R.rs2;
                    switch (rv.immediate >> 6) {
                        case FUNCT7_SRLI:
                            rv.type = RVInstruction::Type::SRLI;
                            break;
                        
                        case FUNCT7_SRAI:
                            rv.type = RVInstruction::Type::SRAI;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_MATH_W_IMMEDIATE:
            rv.rd = iw.I.rd;
            rv.rs1 = iw.I.rs1;
            rv.immediate = iw.I.imm;
            sign_extend_at = 11;

            switch (iw.I.funct3) {
                case FUNCT3_ADDI:
                    rv.type = RVInstruction::Type::ADDIW;
                    break;
                
                case FUNCT3_SLLI:
                    if ((rv.immediate >> 5) != 0) break;
                    rv.type = RVInstruction::Type::SLLIW;
                    break;
                
                case FUNCT3_SHIFT_RIGHT_IMMEDIATE:
                    switch (rv.immediate >> 5) {
                        case FUNCT7_SRLI:
                            rv.type = RVInstruction::Type::SRLIW;
                            break;
                        
                        case FUNCT7_SRAI:
                            rv.type = RVInstruction::Type::SRAIW;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                default:
                    break;
            }
            break;

        case OP_MATH:
            rv.rd = iw.R.rd;
            rv.rs1 = iw.R.rs1;
            rv.rs2 = iw.R.rs2;
            
            switch (iw.R.funct3) {
                case FUNCT3_ADD_SUB_MUL:
                    switch (iw.R.funct7) {
                        case FUNCT7_ADD:
                            rv.type = RVInstruction::Type::ADD;
                            break;
                        
                        case FUNCT7_SUB:
                            rv.type = RVInstruction::Type::SUB;
                            break;
                        
                        case FUNCT7_MUL:
                            rv.type = RVInstruction::Type::MUL;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_SLL_MULH:
                    switch (iw.R.funct7) {
                        case FUNCT7_SLL:
                            rv.type = RVInstruction::Type::SLL;
                            break;
                        
                        case FUNCT7_MULH:
                            rv.type = RVInstruction::Type::MULH;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_SLT_MULHSU:
                    switch (iw.R.funct7) {
                        case FUNCT7_SLT:
                            rv.type = RVInstruction::Type::SLT;
                            break;
                        
                        case FUNCT7_MULHSU:
                            rv.type = RVInstruction::Type::MULHSU;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_SLTU_MULHU:
                    switch (iw.R.funct7) {
                        case FUNCT7_SLTU:
                            rv.type = RVInstruction::Type::SLTU;
                            break;
                        
                        case FUNCT7_MULHU:
                            rv.type = RVInstruction::Type::MULHU;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_XOR_DIV:
                    switch (iw.R.funct7) {
                        case FUNCT7_XOR:
                            rv.type = RVInstruction::Type::XOR;
                            break;
                        
                        case FUNCT7_DIV:
                            rv.type = RVInstruction::Type::DIV;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_SHIFT_RIGHT_DIVU:
                    switch (iw.R.funct7) {
                        case FUNCT7_SRL:
                            rv.type = RVInstruction::Type::SRL;
                            break;
                        
                        case FUNCT7_SRA:
                            rv.type = RVInstruction::Type::SRA;
                            break;
                        
                        case FUNCT7_DIVU:
                            rv.type = RVInstruction::Type::DIVU;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_OR_REM:
                    switch (iw.R.funct7) {
                        case FUNCT7_OR:
                            rv.type = RVInstruction::Type::OR;
                            break;
                        
                        case FUNCT7_REM:
                            rv.type = RVInstruction::Type::REM;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_AND_REMU:
                    switch (iw.R.funct7) {
                        case FUNCT7_AND:
                            rv.type = RVInstruction::Type::AND;
                            break;
                        
                        case FUNCT7_REMU:
                            rv.type = RVInstruction::Type::REMU;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_MATH_W:
            rv.rd = iw.R.rd;
            rv.rs1 = iw.R.rs1;
            rv.rs2 = iw.R.rs2;

            switch (iw.R.funct3) {
                case FUNCT3_ADD_SUB_MUL:
                    switch (iw.R.funct7) {
                        case FUNCT7_ADD:
                            rv.type = RVInstruction::Type::ADDW;
                            break;
                        
                        case FUNCT7_SUB:
                            rv.type = RVInstruction::Type::SUBW;
                            break;
                        
                        case FUNCT7_MUL:
                            rv.type = RVInstruction::Type::MULW;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_SLL_MULH:
                    if (iw.R.funct7 != FUNCT7_SLL) break;
                    rv.type = RVInstruction::Type::SLLW;
                    break;
                
                case FUNCT3_SLT_MULHSU:
                    break;
                
                case FUNCT3_SLTU_MULHU:
                    if (iw.R.funct7 != FUNCT7_SLTU) break;
                    rv.type = RVInstruction::Type::SLTU;
                    break;
                
                case FUNCT3_XOR_DIV:
                    if (iw.R.funct7 != FUNCT7_DIV) break;
                    rv.type = RVInstruction::Type::DIVW;
                    break;
                
                case FUNCT3_SHIFT_RIGHT_DIVU:
                    switch (iw.R.funct7) {
                        case FUNCT7_SRL:
                            rv.type = RVInstruction::Type::SRLW;
                            break;
                        
                        case FUNCT7_SRA:
                            rv.type = RVInstruction::Type::SRAW;
                            break;
                        
                        case FUNCT7_DIVU:
                            rv.type = RVInstruction::Type::DIVUW;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT3_OR_REM:
                    if (iw.R.funct7 != FUNCT7_REM) break;
                    rv.type = RVInstruction::Type::REMW;
                    break;
                
                case FUNCT3_AND_REMU:
                    if (iw.R.funct7 != FUNCT7_REMU) break;
                    rv.type = RVInstruction::Type::REMUW;
                    break;
                
                default:
                    break;
            }
            break;

        case OP_FENCE:
            if (iw.I.funct3 != FUNCT3_FENCE) break;

            rv.type = RVInstruction::Type::FENCE;
            rv.rd = iw.I.rd;
            rv.rs1 = iw.I.rs1;

            break;
        
        case OP_SYSTEM:
            rv.rd = iw.R.rd;
            rv.rs1 = iw.R.rs1;
            rv.rs2 = iw.R.rs2;

            switch (iw.I.funct3) {
                case FUNCT3_SYSTEM:
                    if (iw.I.rd != RD_SYSTEM || iw.I.funct3 != FUNCT3_SYSTEM) break;

                    switch (iw.I.imm) {
                        case IMM_ECALL:
                            if (iw.I.rs1 != RS1_SYSTEM) break;
                            rv.type = RVInstruction::Type::ECALL;
                            break;
                        
                        case IMM_EBREAK:
                            if (iw.I.rs1 != RS1_SYSTEM) break;
                            rv.type = RVInstruction::Type::EBREAK;
                            break;
                        
                        case IMM_SRET:
                            if (iw.I.rs1 != RS1_SYSTEM) break;
                            rv.type = RVInstruction::Type::SRET;
                            break;

                        case IMM_MRET:
                            if (iw.I.rs1 != RS1_SYSTEM) break;
                            rv.type = RVInstruction::Type::MRET;
                            break;
                        
                        case IMM_WFI:
                            if (iw.I.rs1 != RS1_SYSTEM) break;
                            rv.type = RVInstruction::Type::WFI;
                            break;

                        default:
                            switch (iw.R.funct7) {
                                case FUNCT7_SFENCE_VMA:
                                    rv.type = RVInstruction::Type::SFENCE_VMA;
                                    break;
                                
                                case FUNCT7_SINVAL_VMA:
                                    rv.type = RVInstruction::Type::SINVAL_VMA;
                                    break;
                                
                                case FUNCT7_SINVAL_GVMA:
                                    rv.type = RVInstruction::Type::SINVAL_GVMA;
                                    break;
                                
                                case FUNCT7_SFENCE_SINVAL:
                                    if (iw.I.rs1 != RS1_SYSTEM) break;

                                    switch (iw.R.rs2) {
                                        case RS2_SFENCE_W_INVAL:
                                            rv.type = RVInstruction::Type::SFENCE_W_INVAL;
                                            break;
                                        
                                        case RS2_SFENCE_INVAL_IR:
                                            rv.type = RVInstruction::Type::SFENCE_INVAL_IR;
                                            break;
                                        
                                        default:
                                            break;
                                    }
                                    break;
                            }
                            break;
                    }
                    break;
                
                case FUNCT3_CSRRW:
                    rv.type = RVInstruction::Type::CSRRW;
                    break;
                
                case FUNCT3_CSRRS:
                    rv.type = RVInstruction::Type::CSRRS;
                    break;
                
                case FUNCT3_CSRRC:
                    rv.type = RVInstruction::Type::CSRRC;
                    break;
                
                case FUNCT3_CSRRWI:
                    rv.type = RVInstruction::Type::CSRRWI;
                    break;
                
                case FUNCT3_CSRRSI:
                    rv.type = RVInstruction::Type::CSRRSI;
                    break;
                
                case FUNCT3_CSRRCI:
                    rv.type = RVInstruction::Type::CSRRCI;
                    break;
                
                default:
                    break;
            }

            switch (iw.I.funct3) {
                case FUNCT3_CSRRW:
                case FUNCT3_CSRRS:
                case FUNCT3_CSRRC:
                case FUNCT3_CSRRWI:
                case FUNCT3_CSRRSI:
                case FUNCT3_CSRRCI:
                    rv.rd = iw.I.rd;
                    rv.rs1 = iw.I.rs1;
                    rv.immediate = iw.I.imm;
                    break;
                
                default:
                    break;
            }
            break;

        case OP_ATOMIC:
            rv.rd = iw.R.rd;
            rv.rs1 = iw.R.rs1;
            rv.rs2 = iw.R.rs2;

            switch (iw.R.funct3) {
                case FUNCT3_ATOMIC:
                    switch (iw.R.funct7 & FUNCT7_ATOMIC_MASK) {
                        case FUNCT7_LR_W:
                            if (iw.R.rs2 != RS2_LR_W) break;

                            rv.type = RVInstruction::Type::LR_W;
                            break;
                        
                        case FUNCT7_SC_W:
                            rv.type = RVInstruction::Type::SC_W;
                            break;
                        
                        case FUNCT7_AMOSWAP_W:
                            rv.type = RVInstruction::Type::AMOSWAP_W;
                            break;
                        
                        case FUNCT7_AMOADD_W:
                            rv.type = RVInstruction::Type::AMOADD_W;
                            break;
                        
                        case FUNCT7_AMOXOR_W:
                            rv.type = RVInstruction::Type::AMOXOR_W;
                            break;
                        
                        case FUNCT7_AMOAND_W:
                            rv.type = RVInstruction::Type::AMOAND_W;
                            break;
                        
                        case FUNCT7_AMOOR_W:
                            rv.type = RVInstruction::Type::AMOOR_W;
                            break;
                        
                        case FUNCT7_AMOMIN_W:
                            rv.type = RVInstruction::Type::AMOMIN_W;
                            break;
                        
                        case FUNCT7_AMOMAX_W:
                            rv.type = RVInstruction::Type::AMOMAX_W;
                            break;
                        
                        case FUNCT7_AMOMINU_W:
                            rv.type = RVInstruction::Type::AMOMINU_W;
                            break;
                        
                        case FUNCT7_AMOMAXU_W:
                            rv.type = RVInstruction::Type::AMOMAXU_W;
                            break;
                        
                        default:
                            break;
                    }
                    break;
            }
            break;
        
        case OP_FL:
            rv.rd = iw.I.rd;
            rv.rs1 = iw.I.rs1;
            rv.immediate = iw.I.imm;
            sign_extend_at = 11;

            switch (iw.I.funct3) {
                case FUNCT3_FLW:
                    rv.type = RVInstruction::Type::FLW;
                    break;
                
                case FUNCT3_FLD:
                    rv.type = RVInstruction::Type::FLD;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_FS:
            rv.rs1 = iw.S.rs1;
            rv.rs2 = iw.S.rs2;
            rv.immediate = iw.S.imm0;
            rv.immediate |= iw.S.imm1 << 5;
            sign_extend_at = 11;

            switch (iw.S.funct3) {
                case FUNCT3_FSW:
                    rv.type = RVInstruction::Type::FSW;
                    break;
                
                case FUNCT3_FSD:
                    rv.type = RVInstruction::Type::FSD;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_FMADD:
            rv.rd = iw.R4.rd;
            rv.rm = iw.R4.funct3;
            rv.rs1 = iw.R4.rs1;
            rv.rs2 = iw.R4.rs2;
            rv.rs3 = iw.R4.rs3;

            switch (iw.R4.funct2) {
                case FUNCT2_S:
                    rv.type = RVInstruction::Type::FMADD_S;
                    break;
                
                case FUNCT2_D:
                    rv.type = RVInstruction::Type::FMADD_D;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_FMSUB:
            rv.rd = iw.R4.rd;
            rv.rm = iw.R4.funct3;
            rv.rs1 = iw.R4.rs1;
            rv.rs2 = iw.R4.rs2;
            rv.rs3 = iw.R4.rs3;

            switch (iw.R4.funct2) {
                case FUNCT2_S:
                    rv.type = RVInstruction::Type::FMSUB_S;
                    break;
                
                case FUNCT2_D:
                    rv.type = RVInstruction::Type::FMSUB_D;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_FNMSUB:
            rv.rd = iw.R4.rd;
            rv.rm = iw.R4.funct3;
            rv.rs1 = iw.R4.rs1;
            rv.rs2 = iw.R4.rs2;
            rv.rs3 = iw.R4.rs3;

            switch (iw.R4.funct2) {
                case FUNCT2_S:
                    rv.type = RVInstruction::Type::FNMSUB_S;
                    break;
                
                case FUNCT2_D:
                    rv.type = RVInstruction::Type::FNMSUB_D;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_FNMADD:
            rv.rd = iw.R4.rd;
            rv.rm = iw.R4.funct3;
            rv.rs1 = iw.R4.rs1;
            rv.rs2 = iw.R4.rs2;
            rv.rs3 = iw.R4.rs3;

            switch (iw.R4.funct2) {
                case FUNCT2_S:
                    rv.type = RVInstruction::Type::FNMADD_S;
                    break;
                
                case FUNCT2_D:
                    rv.type = RVInstruction::Type::FNMADD_D;
                    break;
                
                default:
                    break;
            }
            break;
        
        case OP_FLOAT:
            rv.rd = iw.R.rd;
            rv.rm = iw.R.funct3;
            rv.rs1 = iw.R.rs1;
            rv.rs2 = iw.R.rs2;
            
            switch (iw.R.funct7 >> 2) {
                case FUNCT5_FADD:
                    switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                        case FUNCT2_S:
                            rv.type = RVInstruction::Type::FADD_S;
                            break;
                        
                        case FUNCT2_D:
                            rv.type = RVInstruction::Type::FADD_D;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FSUB:
                    switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                        case FUNCT2_S:
                            rv.type = RVInstruction::Type::FSUB_S;
                            break;
                        
                        case FUNCT2_D:
                            rv.type = RVInstruction::Type::FSUB_D;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FMUL:
                    switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                        case FUNCT2_S:
                            rv.type = RVInstruction::Type::FMUL_S;
                            break;
                        
                        case FUNCT2_D:
                            rv.type = RVInstruction::Type::FMUL_D;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FDIV:
                    switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                        case FUNCT2_S:
                            rv.type = RVInstruction::Type::FDIV_S;
                            break;
                        
                        case FUNCT2_D:
                            rv.type = RVInstruction::Type::FDIV_D;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FSQRT:
                    if (rv.rs2 != RS2_FSQRT) break;

                    switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                        case FUNCT2_S:
                            rv.type = RVInstruction::Type::FSQRT_S;
                            break;
                        
                        case FUNCT2_D:
                            rv.type = RVInstruction::Type::FSQRT_D;
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FSGNJ:
                    switch (iw.R.funct3) {
                        case FUNCT3_FSGNJ:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FSGNJ_S;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FSGNJ_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case FUNCT3_FSGNJN:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FSGNJN_S;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FSGNJN_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case FUNCT3_FSGNJX:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FSGNJX_S;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FSGNJX_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FMIN_FMAX:
                    switch (iw.R.funct3) {
                        case FUNCT3_FMIN:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FMIN_S;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FMIN_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case FUNCT3_FMAX:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FMAX_S;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FMAX_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FCVT_W:
                    switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                        case FUNCT2_S:
                            switch (iw.R.rs2) {
                                case RS2_FCVT_W:
                                    rv.type = RVInstruction::Type::FCVT_W_S;
                                    break;
                                
                                case RS2_FCVT_WU:
                                    rv.type = RVInstruction::Type::FCVT_WU_S;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case FUNCT2_D:
                            switch (iw.R.rs2) {
                                case RS2_FCVT_W:
                                    rv.type = RVInstruction::Type::FCVT_W_D;
                                    break;
                                
                                case RS2_FCVT_WU:
                                    rv.type = RVInstruction::Type::FCVT_WU_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        default:
                            break;
                    }
                    break;

                case FUNCT5_FCLASS_FMV_X_W:
                    if (iw.R.rs2 == RS2_FMV_X_W && iw.R.funct3 == FUNCT3_FMV_X_W) {
                        switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                            case FUNCT2_S:
                                rv.type = RVInstruction::Type::FMV_X_W;
                                break;
                            
                            case FUNCT2_D:
                                rv.type = RVInstruction::Type::FMV_X_D;
                                break;
                            
                            default:
                                break;
                        }
                        break;
                    }
                    else if (iw.R.rs2 == RS2_FCLASS && iw.R.funct3 == FUNCT3_FCLASS) {
                        switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                            case FUNCT2_S:
                                rv.type = RVInstruction::Type::FCLASS_S;
                                break;
                            
                            case FUNCT2_D:
                                rv.type = RVInstruction::Type::FCLASS_D;
                                break;
                            
                            default:
                                break;
                        }
                        break;
                    }
                    break;
                
                case FUNCT5_FCOMPARE:
                    switch (iw.R.funct3) {
                        case FUNCT3_FEQ:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FEQ_S;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FEQ_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case FUNCT3_FLT:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FLT_S;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FLT_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case FUNCT3_FLE:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FLE_S;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FLE_D;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FCVT:
                    switch (iw.R.rs2) {
                        case RS2_FCVT_W:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FCVT_S_W;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FCVT_D_W;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case RS2_FCVT_WU:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FCVT_S_WU;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FCVT_D_WU;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case RS2_FCVT_L:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FCVT_S_L;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FCVT_D_L;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        case RS2_FCVT_LU:
                            switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                                case FUNCT2_S:
                                    rv.type = RVInstruction::Type::FCVT_S_LU;
                                    break;
                                
                                case FUNCT2_D:
                                    rv.type = RVInstruction::Type::FCVT_D_LU;
                                    break;
                                
                                default:
                                    break;
                            }
                            break;
                        
                        default:
                            break;
                    }
                    break;
                
                case FUNCT5_FMV_W_X:
                    if (iw.R.rs2 != RS2_FMV_W_X || iw.R.funct3 != FUNCT3_FMV_W_X) break;
                    rv.type = RVInstruction::Type::FMV_W_X;
                    break;
                
                case FUNCT5_FCVT_D:
                    switch (iw.R.funct7 & FLOAT_FUNCT2_MASK) {
                        case FUNCT2_S:
                            rv.type = RVInstruction::Type::FCVT_L_S;
                            break;
                        
                        case FUNCT2_D:
                            rv.type = RVInstruction::Type::FCVT_L_D;
                            break;
                        
                        default:
                            break;
                    }
                    break;
            }
            break;
        
        case OP_CUST:
            rv.rd = iw.R.rd;
            rv.rs1 = iw.R.rs1;
            rv.rs2 = iw.R.rs2;
            rv.rd = iw.R.rd;
            
            switch (iw.R.funct7) {
                case FUNCT7_CUST_TVA:
                    rv.type = RVInstruction::Type::CUST_TVA;
                    break;
                
                case FUNCT7_CUST_MTRAP:
                    rv.type = RVInstruction::Type::CUST_MTRAP;
                    break;
                
                case FUNCT7_CUST_STRAP:
                    rv.type = RVInstruction::Type::CUST_STRAP;
                    break;
                
                default:
                    break;
            }
            break;

        default:
            break;
    }

    if (sign_extend_at > 0) {
        Long sign = -1ULL << sign_extend_at;
        if (rv.immediate & (1ULL << sign_extend_at)) rv.immediate |= sign;
    }

    return rv;
}