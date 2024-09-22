#include "Test.hpp"

DEFINE_TESTCASE(SD) {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    auto base = Random<Address>(0x2000, 0xffffffff00000000);
    base &= ~7;

    ADD_RAM(base, 0x3000);

    auto offset = Random<Address>(0, 0xff);
    offset &= ~7;

    auto target = base + offset;

    auto value = Random<Long>(0, LONG_MAX);

    memory.WriteLong(target, LONG_MAX);

    auto sel_rs1 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rs2 = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    
    sel_rs2 = EnsureRegistersAreDifferent(sel_rs1, sel_rs2);

    auto& rs1 = vm.GetRegister(sel_rs1).Value();
    auto& rs2 = vm.GetRegister(sel_rs2).Value();

    rs1.u64 = base;
    rs2.u64 = value;

    memory.WriteWord(0x1000, RV64_S(
        RVInstruction::OP_STORE,
        RVInstruction::FUNCT3_SD,
        sel_rs1,
        sel_rs2,
        offset
    ));

    STEP_VMS(1);

    auto read_value = memory.ReadLong(target);

    ASSERT(read_value == value, "Wrong value stored. Expected {:x}, but got {:x}", value, read_value);

    SUCCESS;
}