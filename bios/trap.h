#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

void mtrap_handler();
void strap_handler();

#define TRAP_PRIVILEGE_USER 0b00
#define TRAP_PRIVILEGE_SUPERVISOR 0b01
#define TRAP_PRIVILEGE_MACHINE 0b11

void trigger_mtrap(uint32_t cause, uint32_t privilege_level);
void trigger_strap(uint32_t cause, uint32_t privilege_level);

#endif