#ifndef TEST_HPP
#define TEST_HPP

#include <vector>
#include <fstream>
#include <iostream>
#include <format>
#include <string>

#include <VirtualMachine.hpp>
#include <Expected.hpp>
#include <RV64.hpp>

class __TestCase {
private:
    static std::vector<__TestCase*> test_cases;

public:
    __TestCase(__TestCase* test_case) { test_cases.push_back(test_case); }

    virtual Expected<bool, std::string> Run() = 0;

    virtual std::string GetDescription() const = 0;

    static void RunTestCases();
};

#define DEFINE_TESTCASE(name, description)\
class __TestCase_##name : public __TestCase {\
public:\
    __TestCase_##name() : __TestCase(this) {}\
    Expected<bool, std::string> Run() override;\
    std::string GetDescription() const override { return description; }\
};\
__TestCase_##name __TestCase_##name##_Impl;\
Expected<bool, std::string> __TestCase_##name::Run()

#define SETUP_MEMORY Memory memory;
#define ADD_RAM(base, size) {\
    auto ram = MemoryRAM::Create(base, size);\
    memory.AddMemoryRegion(std::move(ram));\
}

#define ADD_ROM_BYTES(base, longs) {\
    auto rom = MemoryROM::Create(longs, base);\
    memory.AddMemoryRegion(std::move(rom));\
}

#define ADD_ROM_FILE(base, path) {\
    std::ifstream file(path, std::ios::binary | std::ios::ate);\
    if (!file) {\
        std::cerr << std::format("Cannot open {} for reading\n", path);\
        std::exit(EXIT_FAILURE);\
    }\
    size_t bytes = file.tellg();\
    file.seekg(0);\
    size_t longs = (bytes + 7) & ~7;\
    std::vector<Long> data;\
    data.resize(longs, 0);\
    file.read(reinterpret_cast<char*>(data.data()), bytes);\
    file.close();\
    auto rom = MemoryROM::Create(data, base);\
    memory.AddMemoryRegion(std::move(rom));\
}

#define SETUP_VM(start)\
    std::vector<VirtualMachine> vms;\
    vms.emplace_back(VirtualMachine(memory, start, 0));\
    auto& vm = vms[0];\
    vm.Start();

#define ADD_VM(hart_id, start) {\
    auto new_vm = VirtualMachine(memory, start, hart_id);\
    new_vm.Start();\
    vms.emplace_back(std::move(new_vm));\
}

#define STEP_VMS(steps)\
    for (auto& cur_vm : vms)\
        cur_vm.Step(steps);

#define SUCCESS return true;
#define FAILURE(why) return Unexpected<std::string>(std::string(why));

#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(cond, fail_why)\
    if (!(cond))\
        FAILURE(fail_why);

inline Word RV64_R(auto opcode, auto rd, auto funct3, auto rs1, auto rs2, auto funct7) {
    RVInstructionWord iw;
    iw.R = {opcode, rd, funct3, rs1, rs2, funct7};
    return iw.raw;
}

inline Word RV64_R4(auto opcode, auto rd, auto funct3, auto rs1, auto rs2, auto funct2, auto rs3) {
    RVInstructionWord iw;
    iw.R4 = {opcode, rd, funct3, rs1, rs2, funct2, rs3};
    return iw.raw;
}

inline Word RV64_I(auto opcode, auto rd, auto funct3, auto rs1, auto imm) {
    RVInstructionWord iw;
    iw.I = {opcode, rd, funct3, rs1, imm};
    return iw.raw;
}

inline Word RV64_S(auto opcode, auto funct3, auto rs1, auto rs2, auto imm) {
    RVInstructionWord iw;
    iw.S = {opcode, imm, funct3, rs1, rs2, imm >> 5};
    return iw.raw;
}

inline Word RV64_B(auto opcode, auto funct3, auto rs1, auto rs2, auto imm) {
    RVInstructionWord iw;
    iw.B = {opcode, imm >> 11, imm >> 1, funct3, rs1, rs2, imm >> 5, imm >> 12};
    return iw.raw;
}

inline Word RV64_U(auto opcode, auto rd, auto imm) {
    RVInstructionWord iw;
    iw.U = {opcode, rd, imm};
    return iw.raw;
}

inline Word RV64_J(auto opcode, auto rd, auto imm) {
    RVInstructionWord iw;
    iw.J = {opcode, rd, imm >> 12, imm >> 11, imm >> 1, imm >> 20};
    return iw.raw;
}

#endif