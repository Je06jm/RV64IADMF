#include "Test.hpp"

DEFINE_TESTCASE(REMW) {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    auto lhs = Random<SWord>(SHALF_MIN, SWORD_MAX);
    auto rhs = Random<SWord>(SHALF_MIN, SWORD_MAX);

    auto sel_rs1 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rs2 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rd = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto& rs1 = vm.GetRegister(sel_rs1).Value();
    auto& rs2 = vm.GetRegister(sel_rs2).Value();
    auto& rd = vm.GetRegister(sel_rd).Value();

    rs1.s64 = 0;
    rs2.s64 = 0;
    rs1.s32 = lhs;
    rs2.s32 = rhs;

    auto expected = rs1.s64 % rs2.s64;
    expected = SignExtend(expected, 31);

    memory.WriteWord(0x1000, RV64_R(
        RVInstruction::OP_MATH_W,
        sel_rd,
        RVInstruction::FUNCT3_OR_REM,
        sel_rs1,
        sel_rs2,
        RVInstruction::FUNCT7_REM
    ));

    STEP_VMS(1);

    ASSERT(rd.s64 == expected, " Wrong remw result. Expected {:x}, got {:x}", expected, rd.u64);

    SUCCESS;
}