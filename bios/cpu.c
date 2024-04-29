#include "cpu.h"
#include "machine.h"
#include "printf.h"

extern uint32_t cpu_bootstrap_stack_address;
extern uint32_t cpu_bootstrap_start_address;

extern uint8_t img_end;

void cpu_bootstrap();

uint32_t get_cpus(uint32_t* harts) {
    return machine_call(MACHINE_CALL_GET_CPUS, harts);
}

void start_cpu(uint32_t hart, void (*start_function)(), uint32_t stack_address) {
    cpu_bootstrap_stack_address = stack_address;
    cpu_bootstrap_start_address = (uint32_t)start_function;
    machine_call(MACHINE_CALL_START_CPU, hart, (uint32_t)cpu_bootstrap);

    for (int i = 0; i < 100; i++) {}
}