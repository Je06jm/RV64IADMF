#include "printf.h"
#include "input.h"
#include "phys_memory.h"

#define INPUT_BUFFER_SIZE 8

int main(int argc, const char** argv) {
    printf("%i args\n", argc);

    for (int i = 0; i < argc; i++) {
        printf("\t%i %s\n", i, argv[i]);
    }

    phys_setup();

    return 0;
}