#ifndef MACHINE_H
#define MACHINE_H

#include <stdint.h>

uint32_t machine_call(uint32_t sys, ...);
void machine_break();

#define MACHINE_CALL_COUT 0
#define MACHINE_CALL_CIN 1
#define MACHINE_CALL_EXIT (-1U)

#endif