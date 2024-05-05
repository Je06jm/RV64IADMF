#ifndef CUST_INSTR_H
#define CUST_INSTR_H

#include <stdint.h>

#define CUST_FUNCTION_TVA 0
#define CUST_FUNCTION_MTRAP 1
#define CUST_FUNCTION_STRAP 2

void write_cust_instr(uint32_t* instr, uint8_t function, uint8_t rd, uint8_t rs1, uint8_t rs2);

#endif