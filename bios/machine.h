#ifndef MACHINE_H
#define MACHINE_H

#include <stdint.h>

uint32_t machine_call(uint32_t sys, ...);
void machine_break();

#define MACHINE_CALL_COUT 0
#define MACHINE_CALL_CIN 1
#define MACHINE_CALL_START_CPU (-6U)
#define MACHINE_CALL_GET_CPUS (-5U)
#define MACHINE_CALL_GET_SCREEN_ADDRESS (-4U)
#define MACHINE_CALL_GET_SCREEN_SIZE (-3U)
#define MACHINE_CALL_GET_MEMORY_SIZE (-2U)
#define MACHINE_CALL_EXIT (-1U)

#endif