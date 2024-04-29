#ifndef CPU_H
#define CPU_H

#include <stdint.h>

uint32_t current_hart();
uint32_t get_cpus(uint32_t* harts);
void start_cpu(uint32_t hart, void (*start_function)(), uint32_t stack_address);

#endif