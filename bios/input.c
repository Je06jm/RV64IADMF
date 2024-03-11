#include "input.h"

#include "machine.h"
#include "printf.h"

int read_input(char* buffer, int buffer_size) {
    flush();
    return (int)machine_call(MACHINE_CALL_CIN, buffer, buffer_size);
}