#include "puts.h"

#include "machine.h"

int puts(const char* str) {
    int size = 0;
    while (str[size]) size++;
    machine_call(MACHINE_CALL_COUT, str, size);
    return size;
}