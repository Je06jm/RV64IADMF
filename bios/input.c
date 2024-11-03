#include "input.h"

#include "machine.h"
#include "stdio.h"

int read_input(char* buffer, int buffer_size) {
    fflush(stdin);
    return (int)machine_call(MACHINE_CALL_CIN, buffer, buffer_size);
}