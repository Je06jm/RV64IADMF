#include "Test.hpp"

DEFINE_TESTCASE(SLLIW) {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    auto lhs = Random<Long>(0x0, LONG_MAX);
    auto rhs = Random<Long>(0x0, sizeof(Word) * 8 - 1);

    auto sel_rs1 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rd = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto& rs1 = vm.GetRegister(sel_rs1).Value();
    auto& rd = vm.GetRegister(sel_rd).Value();

    rs1.u64 = lhs;

    memory.WriteWord(0x1000, RV64_I(
        RVInstruction::OP_MATH_W_IMMEDIATE,
        sel_rd,
        RVInstruction::FUNCT3_SLLI,
        sel_rs1,
        rhs
    ));

    STEP_VMS(1);

    Long expected = lhs << rhs;
    expected = SignExtend(expected, 31);


    ASSERT(rd.u64 == expected, "Wrong SLLIW result. Expected {:x}, got {:x}", expected, rd.u64);

    SUCCESS;
}