#ifndef APP_ECALLS_HPP
#define APP_ECALLS_HPP

#include <cstdint>

constexpr uint32_t ECALL_COUT = 0;
constexpr uint32_t ECALL_CIN = 1;
constexpr uint32_t ECALL_EXIT = -1U;

void RegisterECalls();

#endif