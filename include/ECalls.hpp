#ifndef ECALLS_HPP
#define ECALLS_HPP

#include <cstdint>

constexpr uint32_t ECALL_COUT = 0;
constexpr uint32_t ECALL_CIN = 1;
constexpr uint32_t ECALL_EXIT = -1U;

void RegisterBuiltinECalls();

#endif