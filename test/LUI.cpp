#include "Test.hpp"

DEFINE_TESTCASE(LUI, "LUI") {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    auto value = Random<uint64_t>(0, UINT32_MAX);

    memory.WriteWord(0x1000, RV64_U(
        RVInstruction::OP_LUI,
        VirtualMachine::REG_A0,
        static_cast<uint32_t>(value)
    ));

    value <<= 12;

    value &= 0xffffffff;

    if (value & 0x80000000LL)
        value |= 0xffffffff00000000LL;

    STEP_VMS(1);

    auto& reg = vm.GetRegister(VirtualMachine::REG_A0).Value();
    ASSERT(reg.u64 == value, "Produces the wrong result. Expecting {:x}, got {:x}", value, reg.u64);

    SUCCESS;
}