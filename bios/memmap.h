#ifndef MEMMAP_H
#define MEMMAP_H

#include <stddef.h>
#include <stdint.h>

#define MEM_REGION_TYPE_UNKNOWN -1ULL
#define MEM_REGION_TYPE_UNUSED 0ULL
#define MEM_REGION_TYPE_PMA_ROM 1ULL
#define MEM_REGION_TYPE_MAPPED_CSRS 2ULL
#define MEM_REGION_TYPE_BIOS_ROM 4ULL
#define MEM_REGION_TYPE_GENERAL_RAM 5ULL
#define MEM_REGION_TYPE_FRAMEBUFFER 8ULL

#define MEM_REGION_FLAG_READABLE (1ULL << 0)
#define MEM_REGION_FLAG_WRITABLE (1ULL << 1)

typedef struct {
    uint64_t type;
    uint64_t base, size;
    uint64_t flags;
} mem_region;

size_t mem_region_count();
void get_mem_region(size_t index, mem_region** region);
size_t find_mem_region(uint64_t type);

#define MEM_REGION_NOT_FOUND -1ULL

#endif