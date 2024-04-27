#ifndef APP_ECALLS_HPP
#define APP_ECALLS_HPP

#include <cstdint>

// void ecall_cout(const char* buffer, uint32_t count);
constexpr uint32_t ECALL_COUT = 0;

// uint32_t ecall_cin(const char* buffer, uint32_t buffer_size);
constexpr uint32_t ECALL_CIN = 1;

// uint32_t ecall_get_memory_size();
constexpr uint32_t ECALL_GET_MEMORY_SIZE = -2U;

// void ecall_exit(uint32_t exit_code)
constexpr uint32_t ECALL_EXIT = -1U;

void RegisterECalls();

#endif