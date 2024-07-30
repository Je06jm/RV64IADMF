#ifndef TEST_HPP
#define TEST_HPP

#include <vector>
#include <fstream>
#include <iostream>
#include <format>
#include <string>
#include <type_traits>

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

    static void RunTestCases(size_t iterations = 50);
};

#define DEFINE_TESTCASE(name)\
class __TestCase_##name : public __TestCase {\
public:\
    __TestCase_##name() : __TestCase(this) {}\
    Expected<bool, std::string> Run() override;\
    std::string GetDescription() const override { return #name; }\
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
#define FAILURE(...) return Unexpected<std::string>(std::format(__VA_ARGS__));

#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(cond, ...)\
    if (!(cond))\
        FAILURE(__VA_ARGS__);

inline Word RV64_R(Word opcode, Word rd, Word funct3, Word rs1, Word rs2, Word funct7) {
    RVInstructionWord iw;
    iw.R = {opcode, rd, funct3, rs1, rs2, funct7};
    return iw.raw;
}

inline Word RV64_R4(Word opcode, Word rd, Word funct3, Word rs1, Word rs2, Word funct2, Word rs3) {
    RVInstructionWord iw;
    iw.R4 = {opcode, rd, funct3, rs1, rs2, funct2, rs3};
    return iw.raw;
}

inline Word RV64_I(Word opcode, Word rd, Word funct3, Word rs1, Word imm) {
    RVInstructionWord iw;
    iw.I = {opcode, rd, funct3, rs1, imm};
    return iw.raw;
}

inline Word RV64_S(Word opcode, Word funct3, Word rs1, Word rs2, Word imm) {
    RVInstructionWord iw;
    iw.S = {opcode, imm, funct3, rs1, rs2, imm >> 5};
    return iw.raw;
}

inline Word RV64_B(Word opcode, Word funct3, Word rs1, Word rs2, Word imm) {
    RVInstructionWord iw;
    iw.B = {opcode, imm >> 11, imm >> 1, funct3, rs1, rs2, imm >> 5, imm >> 12};
    return iw.raw;
}

inline Word RV64_U(Word opcode, Word rd, Word imm) {
    RVInstructionWord iw;
    iw.U = {opcode, rd, imm};
    return iw.raw;
}

inline Word RV64_J(Word opcode, Word rd, Word imm) {
    RVInstructionWord iw;
    iw.J = {opcode, rd, imm >> 12, imm >> 11, imm >> 1, imm >> 20};
    return iw.raw;
}

size_t RandomInt();

template <typename Type>
inline Type Random(Type min, Type max) {
    if constexpr (std::is_floating_point_v<Type>) {
        long double num = RandomInt() / static_cast<long double>(UINT64_MAX);
        return (num * (max - min)) + min;
    }
    else if constexpr (std::is_integral_v<Type>) {
        size_t num = RandomInt();

        if constexpr (std::is_signed_v<Type>) {
            num &= ~(1ULL << (sizeof(size_t) * 8 - 1));
            ssize_t snum = num;
            return (snum - min) % (max - min) + min;
        }
        else {
            return (num - min) % (max - min) + min;
        }
    }
    else {
        static_assert(false);
    }
}

template <typename Type>
inline Type SignExtend(Type value, auto bit) {
    if constexpr (sizeof(value) < sizeof(uint64_t)) {
        if (value & (1U << bit))
            value |= 0xffffffff << bit;
    }
    else if constexpr (sizeof(value) == sizeof(uint64_t)) {
        if (value & (1ULL << bit))
            value |= 0xffffffffffffffff << bit;
    }
    return value;
}

#endif