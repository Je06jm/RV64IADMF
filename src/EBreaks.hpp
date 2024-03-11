#ifndef EBREAKS_HPP
#define EBREAKS_HPP

#include <cstdint>

constexpr uint32_t EBREAK_COUT = 0;
constexpr uint32_t EBREAK_CIN = 1;

void RegisterBuiltinEBreaks();

#endif