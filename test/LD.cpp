#include "Test.hpp"

DEFINE_TESTCASE(LD) {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    auto base = Random<Address>(0x2000, 0xffffffffffffe000);
    base &= ~7;

    ADD_RAM(base, 0x3000);

    auto offset = Random<Address>(0, 0xff);
    offset &= ~7;
    
    auto target = base + offset;

    auto value = Random<Long>(0, LONG_MAX);

    memory.WriteLong(target, value);

    auto sel_rs1 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rd = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto& rs1 = vm.GetRegister(sel_rs1).Value();
    auto& rd = vm.GetRegister(sel_rd).Value();

    rs1.u64 = base;

    memory.WriteWord(0x1000, RV64_I(
        RVInstruction::OP_LOAD,
        sel_rd,
        RVInstruction::FUNCT3_LD,
        sel_rs1,
        offset
    ));

    STEP_VMS(1);

    ASSERT(rd.u64 == value, "Wrong value loaded. Expected {:x}, got {:x}", value, rd.u64);

    SUCCESS;
}