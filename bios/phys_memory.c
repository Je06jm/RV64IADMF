#include "phys_memory.h"

#include "machine.h"

#include "printf.h"

#include <stddef.h>
#include <stdbool.h>

uint32_t memory_size;

extern char img_begin;
extern char img_end;

#define BUDDY_USED (1U<<0)
#define BUDDY_ALLOCATED (1U<<1)

typedef struct PhysBuddy {
    struct PhysBuddy* left;
    struct PhysBuddy* right;
    uint32_t flags;
} PhysBuddy;

PhysBuddy* phys_root = NULL;
PhysBuddy* phys_buddies = NULL;
uint32_t phys_buddies_count;

void phys_setup() {
    memory_size = machine_call(MACHINE_CALL_GET_MEMORY_SIZE);

    phys_buddies_count = 1;
    {
        uint32_t size = memory_size;
        while (size > PHYS_MIN_ALLOC_SIZE) {
            phys_buddies_count++;
            size /= 2;
        }
    }

    phys_buddies = (PhysBuddy*)((((uint32_t)&img_end) + 7) & ~3);
    for (uint32_t i = 0; i < phys_buddies_count; i++) {
        phys_buddies[i].flags = 0;
    }

    phys_root = &phys_buddies[0];
    
    phys_root->flags = BUDDY_ALLOCATED;
    phys_root->left = NULL;
    phys_root->right = NULL;

    // 
}

uint32_t phys_alloc();