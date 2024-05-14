#include "printf.h"
#include "input.h"
#include "machine.h"

int main(int argc, const char** argv) {
    printf("Found %i args\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("Arg %i: %s\n", i, argv[i]);
    }
    while (1) {}
    return 0;
}