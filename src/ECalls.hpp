#ifndef ECALLS_HPP
#define ECALLS_HPP

#include <cstdint>

constexpr uint32_t ECALL_COUT = 0;
constexpr uint32_t ECALL_CIN = 1;

void RegisterBuiltinECalls();

#endif