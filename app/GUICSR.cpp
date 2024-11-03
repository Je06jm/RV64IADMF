#include "GUICSR.hpp"

#include <imgui.h>

#include <unordered_map>
#include <format>
#include <array>
#include <tuple>
#include <string>

void GUICSR::Draw() {
    using CSRTuple = std::tuple<Word, std::string>;
    using VM = VirtualMachine;
    
    static std::array csrs_names = {
        CSRTuple(VM::CSR_FFLAGS, "fflags"),
        CSRTuple(VM::CSR_FRM, "frm"),
        CSRTuple(VM::CSR_FCSR, "fcsr"),
        CSRTuple(VM::CSR_CYCLE, "cycle"),
        CSRTuple(VM::CSR_TIME, "time"),
        CSRTuple(VM::CSR_INSTRET, "instret"),
        CSRTuple(VM::CSR_SSTATUS, "sstatus"),
        CSRTuple(VM::CSR_SIE, "sie"),
        CSRTuple(VM::CSR_STVEC, "stvec"),
        CSRTuple(VM::CSR_SENVCFG, "senvcfg"),
        CSRTuple(VM::CSR_SSCRATCH, "sscratch"),
        CSRTuple(VM::CSR_SEPC, "sepc"),
        CSRTuple(VM::CSR_SCAUSE, "scause"),
        CSRTuple(VM::CSR_STVAL, "stval"),
        CSRTuple(VM::CSR_SIP, "sip"),
        CSRTuple(VM::CSR_SATP, "satp"),
        CSRTuple(VM::CSR_MVENDORID, "mvendorid"),
        CSRTuple(VM::CSR_MARCHID, "marchid"),
        CSRTuple(VM::CSR_MIMPID, "mimpid"),
        CSRTuple(VM::CSR_MHARTID, "mhartid"),
        CSRTuple(VM::CSR_MCONFIGPTR, "mconfigptr"),
        CSRTuple(VM::CSR_MSTATUS, "mstatus"),
        CSRTuple(VM::CSR_MISA, "misa"),
        CSRTuple(VM::CSR_MEDELEG, "medeleg"),
        CSRTuple(VM::CSR_MIDELEG, "mideleg"),
        CSRTuple(VM::CSR_MIE, "mie"),
        CSRTuple(VM::CSR_MTVEC, "mtvec"),
        CSRTuple(VM::CSR_MSCRATCH, "mscratch"),
        CSRTuple(VM::CSR_MEPC, "mepc"),
        CSRTuple(VM::CSR_MCAUSE, "mcause"),
        CSRTuple(VM::CSR_MTVAL, "mtval"),
        CSRTuple(VM::CSR_MIP, "mip"),
        CSRTuple(VM::CSR_MTINST, "mtinst"),
        CSRTuple(VM::CSR_MTVAL2, "mtval2"),
        CSRTuple(VM::CSR_MENVCFG, "menvcfg"),
        CSRTuple(VM::CSR_MSECCFG, "mseccfg"),
        CSRTuple(VM::CSR_MCYCLE, "mcycle"),
        CSRTuple(VM::CSR_MINSTRET, "minstret"),
    };

    if (ImGui::Begin("CSRs")) {
        std::unordered_map<Long, Long> csrs;
        vm->GetCSRSnapshot(csrs);

        for (const auto& [csr, name] : csrs_names) {
            auto fmt = std::format("{:<16}0x{:<3x} : 0x{:0>16x} ({})", name, csr, csrs[csr], csrs[csr]);
            ImGui::Text("%s", fmt.c_str());
        }
    }

    ImGui::End();
}