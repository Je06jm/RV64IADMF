#ifndef APP_ECALLS_HPP
#define APP_ECALLS_HPP

#include <cstdint>

#include <Types.hpp>

// void ecall_cout(const char* buffer, Long count);
constexpr Long ECALL_COUT = 0ULL;

// Long ecall_cin(const char* buffer, Long buffer_size);
constexpr Long ECALL_CIN = 1ULL;

// void ecall_start_cpu(Long hart, Long pc);
constexpr Long ECALL_START_CPU = -6ULL;

// Long ecall_get_cpus(Hart* harts);
constexpr Long ECALL_GET_CPUS = -5ULL;

// Long ecall_get_screen_address();
constexpr Long ECALL_GET_SCREEN_ADDRESS = -4ULL;

// void ecall_get_screen_size(Long* x, Long* y);
constexpr Long ECALL_GET_SCREEN_SIZE = -3ULL;

// Long ecall_get_memory_size();
constexpr Long ECALL_GET_MEMORY_SIZE = -2ULL;

// void ecall_exit(Long exit_code)
constexpr Long ECALL_EXIT = -1ULL;

void RegisterECalls();

#endif