#include "Test.hpp"

DEFINE_TESTCASE(JAL, "JAL") {
    auto base = Random<Address>(0x10000, 0x100000);
    base &= ~3;

    SETUP_MEMORY;
    SETUP_VM(base);

    ADD_RAM(base, 0x1000);

    auto reg = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto jump = Random<Address>(0, UINT32_MAX);
    jump &= ~3;
    jump <<= 12;
    jump &= 0xffffffff;
    jump >>= 12;

    memory.WriteWord(base, RV64_J(
        RVInstruction::OP_JAL,
        reg,
        static_cast<uint32_t>(jump)
    ));

    jump = SignExtend(jump, 20);

    STEP_VMS(1);

    auto& reg_val = vm.GetRegister(reg).Value();
    ASSERT(reg_val.u64 == (base + 4), "JAL did not store the correct return address. Expecting {:x} in {}, got {:x}", (base + 4), reg, reg_val.u64);

    auto pc = vm.GetPC();
    ASSERT(pc == (base + jump), "JAL did not jump to the correct address. Expecting {:x}, got {:x}", (base + jump), pc);

    SUCCESS;
}