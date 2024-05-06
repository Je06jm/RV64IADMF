#include "virt_mem.h"

#include "machine.h"

#include <stdint.h>

extern void set_vtable_addr(uint32_t address);
extern void enable_smode();

uint32_t* virt_table = (uint32_t*)0x100000;

typedef union {
    struct {
        uint32_t V : 1;
        uint32_t R : 1;
        uint32_t W : 1;
        uint32_t X : 1;
        uint32_t U : 1;
        uint32_t G : 1;
        uint32_t A : 1;
        uint32_t D : 1;
        uint32_t RSW : 2;
        uint32_t PPN_0 : 10;
        uint32_t PPN_1 : 12;
    };
    struct {
        uint32_t _unused : 10;
        uint32_t PPN : 22;
    };
    uint32_t raw;
} VirtEntry;

void virt_mem_setup() {
    for (uint32_t i = 0; i < 512; i++) {
        VirtEntry* ve = (VirtEntry*)(&virt_table[i]);
        ve->raw = 0;

        ve->PPN = i;
        ve->V = 1;
        ve->W = 1;
        ve->R = 1;
        ve->X = 1;
        ve->A = 1;
    }

    set_vtable_addr((uint32_t)virt_table >> 12);
    enable_smode();
}