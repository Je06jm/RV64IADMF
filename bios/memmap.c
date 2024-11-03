#include "memmap.h"

size_t mem_region_count() {
    mem_region* regions = (mem_region*)0;
    size_t index = 0;
    while (regions[index].type != MEM_REGION_TYPE_UNUSED)
        index++;
    
    return index;
}

void get_mem_region(size_t index, mem_region** region) {
    if (!region)
        return;
    
    *region = &((mem_region*)0)[index];
}

size_t find_mem_region(uint64_t type) {
    mem_region* regions = (mem_region*)0;
    for (size_t i = 0; regions[i].type != MEM_REGION_TYPE_UNUSED; i++) {
        if (regions[i].type == type)
            return i;
    }

    return MEM_REGION_NOT_FOUND;
}