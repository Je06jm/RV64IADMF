#include "Test.hpp"

DEFINE_TESTCASE(AUIPC, "AUIPC") {
    auto base = Random<Address>(0x1000, 0xffffffffffffe000);
    base &= ~3ULL;

    SETUP_MEMORY;
    SETUP_VM(base);

    ADD_RAM(base, 0x1000);

    auto reg = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto value = Random<uint64_t>(0, UINT32_MAX);

    value <<= 12;
    value &= 0xffffffff;

    memory.WriteWord(base, RV64_U(
        RVInstruction::OP_AUIPC,
        reg,
        static_cast<uint32_t>(value >> 12)
    ));

    value = SignExtend(value, 31);
    
    STEP_VMS(1);

    auto& reg_val = vm.GetRegister(reg).Value();
    ASSERT(reg_val.u64 == (base + value), "Expecting {:x} in register {}, got {:x}", value, reg, reg_val.u64);

    SUCCESS;
}