#include "machine.h"
#include "printf.h"

#include "cust_instr.h"

void mtrap_handler() {
    printf("MTrap handler\n");
}

void strap_handler() {
    printf("STrap handler\n");
}

void cust_mtrap(uint32_t cause, uint32_t privilege_level);
void cust_strap(uint32_t cause, uint32_t privilege_level);

void trigger_mtrap(uint32_t cause, uint32_t privilege_level) {
    uint32_t* mtrap_instr = (uint32_t*)cust_mtrap;
    write_cust_instr(&mtrap_instr[3], CUST_FUNCTION_MTRAP, 0, 0b01010, 0b01011);
    cust_mtrap(cause, privilege_level);
}

void trigger_strap(uint32_t cause, uint32_t privilege_level) {
    uint32_t* strap_instr = (uint32_t*)cust_strap;
    write_cust_instr(&strap_instr[3], CUST_FUNCTION_STRAP, 0, 0b01010, 0b01011);
    cust_strap(cause, privilege_level);
}