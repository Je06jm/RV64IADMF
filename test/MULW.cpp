#include "Test.hpp"

DEFINE_TESTCASE(MULW) {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    auto lhs = Random<Word>(0x0, WORD_MAX);
    auto rhs = Random<Word>(0x0, WORD_MAX);

    auto sel_rs1 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rs2 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rd = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto& rs1 = vm.GetRegister(sel_rs1).Value();
    auto& rs2 = vm.GetRegister(sel_rs2).Value();
    auto& rd = vm.GetRegister(sel_rd).Value();

    rs1.u64 = 0;
    rs2.u64 = 0;
    rs1.u32 = lhs;
    rs2.u32 = rhs;

    auto expected = rs1.s64 * rs2.s64;
    expected &= 0xffffffff;
    expected = SignExtend(expected, 31);

    memory.WriteWord(0x1000, RV64_R(
        RVInstruction::OP_MATH_W,
        sel_rd,
        RVInstruction::FUNCT3_ADD_SUB_MUL,
        sel_rs1,
        sel_rs2,
        RVInstruction::FUNCT7_MUL
    ));

    STEP_VMS(1);

    ASSERT(rd.s64 == expected, " Wrong mulw result. Expected {:x}, got {:x}", expected, rd.u64);

    SUCCESS;
}