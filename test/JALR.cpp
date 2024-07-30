#include "Test.hpp"

DEFINE_TESTCASE(JALR) {
    auto base = Random<Address>(0x1000, 0xffffffff00000000);
    base &= ~3;
    
    auto jump = Random<Address>(0x4, 0xffffffff);
    jump &= ~3;
    jump &= 0xfff;

    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);


    auto sel_rd = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rs1 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto& rd = vm.GetRegister(sel_rd).Value();
    auto& rs1 = vm.GetRegister(sel_rs1).Value();

    rs1.u64 = base;

    memory.WriteWord(0x1000, RV64_I(
        RVInstruction::OP_JALR,
        sel_rd,
        RVInstruction::FUNCT3_JALR,
        sel_rs1,
        jump
    ));

    jump = SignExtend(jump, 11);

    STEP_VMS(1);

    ASSERT(rd.u64 == 0x1004, "JALR did not store the correct return address. Expecting {:x} in {}, got {:x}", 0x1004, sel_rd, rd.u64);

    auto pc = vm.GetPC();
    ASSERT(pc == (base + jump), "JALR did not jump to the correct address. Expecting {:x}, got {:x}", (base + jump), pc);

    SUCCESS;
}