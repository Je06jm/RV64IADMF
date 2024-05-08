#include <GUICSR.hpp>

#include <imgui.h>

#include <unordered_map>
#include <format>
#include <array>
#include <tuple>
#include <string>

void GUICSR::Draw() {
    using CSRTuple = std::tuple<Word, std::string, bool>;
    using VM = VirtualMachine;
    
    static std::array csrs_names = {
        CSRTuple(VM::CSR_FFLAGS, "fflags", false),
        CSRTuple(VM::CSR_FRM, "frm", false),
        CSRTuple(VM::CSR_FCSR, "fcsr", false),
        CSRTuple(VM::CSR_CYCLE, "cycle", false),
        CSRTuple(VM::CSR_TIME, "time", false),
        CSRTuple(VM::CSR_INSTRET, "instret", false),
        CSRTuple(VM::CSR_CYCLEH, "cycleh", false),
        CSRTuple(VM::CSR_TIMEH, "timeh", false),
        CSRTuple(VM::CSR_INSTRETH, "instreth", false),
        CSRTuple(VM::CSR_SSTATUS, "sstatus", false),
        CSRTuple(VM::CSR_SIE, "sie", false),
        CSRTuple(VM::CSR_STVEC, "stvec", false),
        CSRTuple(VM::CSR_SENVCFG, "senvcfg", false),
        CSRTuple(VM::CSR_SSCRATCH, "sscratch", false),
        CSRTuple(VM::CSR_SEPC, "sepc", false),
        CSRTuple(VM::CSR_SCAUSE, "scause", false),
        CSRTuple(VM::CSR_STVAL, "stval", false),
        CSRTuple(VM::CSR_SIP, "sip", false),
        CSRTuple(VM::CSR_SATP, "satp", false),
        CSRTuple(VM::CSR_MVENDORID, "mvendorid", false),
        CSRTuple(VM::CSR_MARCHID, "marchid", false),
        CSRTuple(VM::CSR_MIMPID, "mimpid", false),
        CSRTuple(VM::CSR_MHARTID, "mhartid", false),
        CSRTuple(VM::CSR_MCONFIGPTR, "mconfigptr", false),
        CSRTuple(VM::CSR_MSTATUS, "mstatus", false),
        CSRTuple(VM::CSR_MISA, "misa", true),
        CSRTuple(VM::CSR_MEDELEG, "medeleg", false),
        CSRTuple(VM::CSR_MIDELEG, "mideleg", false),
        CSRTuple(VM::CSR_MIE, "mie", false),
        CSRTuple(VM::CSR_MTVEC, "mtvec", false),
        CSRTuple(VM::CSR_MSTATUSH, "mstatush", false),
        CSRTuple(VM::CSR_MSCRATCH, "mscratch", false),
        CSRTuple(VM::CSR_MEPC, "mepc", false),
        CSRTuple(VM::CSR_MCAUSE, "mcause", false),
        CSRTuple(VM::CSR_MTVAL, "mtval", false),
        CSRTuple(VM::CSR_MIP, "mip", false),
        CSRTuple(VM::CSR_MTINST, "mtinst", false),
        CSRTuple(VM::CSR_MTVAL2, "mtval2", false),
        CSRTuple(VM::CSR_MENVCFG, "menvcfg", false),
        CSRTuple(VM::CSR_MENVCFGH, "menvcfgh", false),
        CSRTuple(VM::CSR_MSECCFG, "mseccfg", false),
        CSRTuple(VM::CSR_MSECCFGH, "mseccfgh", false),
        CSRTuple(VM::CSR_MCYCLE, "mcycle", false),
        CSRTuple(VM::CSR_MINSTRET, "minstret", false),
        CSRTuple(VM::CSR_MCYCLEH, "mcycleh", false),
        CSRTuple(VM::CSR_MINSTRETH, "minstreth", false),
    };

    if (ImGui::Begin("CSRs")) {
        std::unordered_map<Word, Word> csrs;
        vm->GetCSRSnapshot(csrs);

        for (const auto& [csr, name, binary] : csrs_names) {
            if (binary)
                ImGui::Text("%-16s0x%-3x : 0x%08x (%s)", name.c_str(), csr, csrs[csr], std::format("{:b}", csrs[csr]).c_str());
            else
                ImGui::Text("%-16s0x%-3x : 0x%08x (%u)", name.c_str(), csr, csrs[csr], csrs[csr]);
        }
    }

    ImGui::End();
}