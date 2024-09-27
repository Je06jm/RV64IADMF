#include "Test.hpp"

DEFINE_TESTCASE(CSRRSI) {
    SETUP_MEMORY;
    SETUP_VM(0x1000);
    
    ADD_RAM(0x1000, 0x1000);

    Long csrs[] = {VirtualMachine::CSR_SSCRATCH, VirtualMachine::CSR_SCAUSE, VirtualMachine::CSR_SCAUSE};

    auto sel_rs = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);
    auto sel_rd = Random<size_t>(1, VirtualMachine::REGISTER_COUNT);

    auto sel_csr = Random<Long>(0, 2);
    sel_csr = csrs[sel_csr];

    auto& rd = vm.GetRegister(sel_rd).Value();
    
    std::unordered_map<Long, Long> csrs_original;
    vm.GetCSRSnapshot(csrs_original);

    memory.WriteWord(0x1000, RV64_I(
        RVInstruction::OP_CSR,
        sel_rd,
        RVInstruction::FUNCT3_CSRRSI,
        sel_rs,
        sel_csr
    ));

    STEP_VMS(1);

    std::unordered_map<Long, Long> csrs_updated;
    vm.GetCSRSnapshot(csrs_updated);
    auto expected = csrs_original[sel_csr];

    ASSERT(rd.u64 == expected, "Wrong CSRSW result. Expected {:x} in RD {:x}, got {:x}", expected, sel_rd, rd.u64);

    expected |= sel_rs;
    ASSERT(csrs_updated[sel_csr] == expected, "Wrong CSRSW result. Expected {:x} in CSR {:x}, got {:x}", expected, sel_csr, csrs_updated[sel_csr]);

    SUCCESS;
}