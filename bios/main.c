#include "printf.h"
#include "input.h"
#include "machine.h"
#include "trap.h"

int main(int argc, const char** argv) {
    //phys_setup();

    trigger_mtrap(0x12, TRAP_PRIVILEGE_USER);

    printf("Hello!\n");

    while (1) {}
    
    return 0;
}