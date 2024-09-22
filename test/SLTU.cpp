#include "Test.hpp"

DEFINE_TESTCASE(SLTU) {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    auto lhs = Random<Long>(0x0, LONG_MAX);
    auto rhs = Random<Long>(0x0, sizeof(Long) * 8);

    auto sel_rs1 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rs2 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rd = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto& rs1 = vm.GetRegister(sel_rs1).Value();
    auto& rs2 = vm.GetRegister(sel_rs2).Value();
    auto& rd = vm.GetRegister(sel_rd).Value();

    rs1.u64 = lhs;
    rs2.u64 = rhs;

    Long expected = rs1.u64 < rs2.u64 ? 1 : 0;

    memory.WriteWord(0x1000, RV64_R(
        RVInstruction::OP_MATH,
        sel_rd,
        RVInstruction::FUNCT3_SLTU_MULHU,
        sel_rs1,
        sel_rs2,
        RVInstruction::FUNCT7_SLTU
    ));

    STEP_VMS(1);

    ASSERT(rd.u64 == expected, " Wrong SLTU result. Expected {:x}, got {:x}", expected, rd.u64);

    SUCCESS;
}