#include "Test.hpp"

DEFINE_TESTCASE(BEQ, "BEQ") {
    auto base = Random<Address>(0x1000, 0xffffffff00000000);
    base &= ~3;

    SETUP_MEMORY;
    SETUP_VM(base);

    ADD_RAM(base, 0x1000);

    auto sel_rs1 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rs2 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto& rs1 = vm.GetRegister(sel_rs1).Value();
    auto& rs2 = vm.GetRegister(sel_rs2).Value();

    rs1.u64 = Random<Long>(0, LONG_MAX);
    rs2.u64 = rs1.u64;

    auto jump = Random<Address>(0, 0x1fff);
    jump &= ~3;

    jump &= 0x1fff;

    memory.WriteWord(base, RV64_B(
        RVInstruction::OP_BRANCH,
        RVInstruction::FUNCT3_BEQ,
        sel_rs1,
        sel_rs2,
        jump
    ));

    jump = SignExtend(jump, 12);

    STEP_VMS(1);

    ASSERT(vm.GetPC() == (base + jump), "BEQ {} == {} did not jump. Expected {:x}, got {:x}", sel_rs1, sel_rs2, (base + jump), vm.GetPC());

    if (sel_rs1 == sel_rs2)
        SUCCESS;

    vm.SetPC(base);

    rs2.u64 = rs1.u64 ^ Random<Long>(1, LONG_MAX);

    STEP_VMS(1);

    ASSERT(vm.GetPC() == (base + 4), "BEQ {} != {} did jump. Expected {:x}, got {:x}", sel_rs1, sel_rs2, (base + 4), vm.GetPC());

    SUCCESS;
}