#include "Test.hpp"

DEFINE_TESTCASE(LUI, "LUI") {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    auto reg = Random<size_t>(0, VirtualMachine::REGISTER_COUNT);
    auto value = Random<uint64_t>(0, UINT32_MAX);

    memory.WriteWord(0x1000, RV64_U(
        RVInstruction::OP_LUI,
        reg,
        static_cast<uint32_t>(value)
    ));

    value <<= 12;

    value &= 0xffffffff;

    if (value & 0x80000000LL)
        value |= 0xffffffff00000000LL;

    STEP_VMS(1);

    auto& reg_val = vm.GetRegister(reg).Value();
    ASSERT(reg_val.u64 == value, "Produces the wrong result. Expecting {:x} in register {}, got {:x}", value, reg, reg_val.u64);

    SUCCESS;
}