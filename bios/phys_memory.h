#ifndef PHYS_MEMORY_H
#define PHYS_MEMORY_H

#include <stdint.h>

#define PHYS_MIN_ALLOC_SIZE 16

void phys_setup();

uint32_t phys_alloc(uint32_t size);
void phys_free(uint32_t address, uint32_t size);

#endif