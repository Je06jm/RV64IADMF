#include "printf.h"
#include "input.h"
#include "machine.h"
#include "virt_mem.h"

int main(int argc, const char** argv) {
    //phys_setup();

    virt_mem_setup();

    printf("Hello!\n");

    printf("This is the Supervisor!\n");

    while (1) {}
    
    return 0;
}