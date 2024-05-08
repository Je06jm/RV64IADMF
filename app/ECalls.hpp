#ifndef APP_ECALLS_HPP
#define APP_ECALLS_HPP

#include <cstdint>

#include <Types.hpp>

// void ecall_cout(const char* buffer, Word count);
constexpr Word ECALL_COUT = 0;

// Word ecall_cin(const char* buffer, Word buffer_size);
constexpr Word ECALL_CIN = 1;

// void ecall_start_cpu(Word hart, Word pc);
constexpr Word ECALL_START_CPU = -6U;

// Word ecall_get_cpus(Hart* harts);
constexpr Word ECALL_GET_CPUS = -5U;

// Word ecall_get_screen_address();
constexpr Word ECALL_GET_SCREEN_ADDRESS = -4U;

// void ecall_get_screen_size(Word* x, Word* y);
constexpr Word ECALL_GET_SCREEN_SIZE = -3U;

// Word ecall_get_memory_size();
constexpr Word ECALL_GET_MEMORY_SIZE = -2U;

// void ecall_exit(Word exit_code)
constexpr Word ECALL_EXIT = -1U;

void RegisterECalls();

#endif