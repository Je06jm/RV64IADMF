#include "Test.hpp"

DEFINE_TESTCASE(LUI, "LUI") {
    SETUP_MEMORY;
    SETUP_VM(0x1000);

    ADD_RAM(0x1000, 0x1000);

    memory.WriteWord(0x1000, RV64_U(
        RVInstruction::OP_LUI,
        VirtualMachine::REG_A0,
        0x1
    ));

    STEP_VMS(1);

    ASSERT(vm.GetRegister(VirtualMachine::REG_A0).Value().u32 == 0x1000, "Produces the wrong result");

    SUCCESS;
}