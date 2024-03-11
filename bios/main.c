#include "printf.h"

#include "input.h"

#include "machine.h"

#define INPUT_BUFFER_SIZE 8

int main(int argc, const char** argv) {
    char buffer[INPUT_BUFFER_SIZE];
    
    printf("What is your name? ");
    int chars = read_input(buffer, INPUT_BUFFER_SIZE);

    if ((chars + 1) >= INPUT_BUFFER_SIZE) {
        printf("Could not read input as it is too big for the buffer\n");
        return -1;
    }

    else if (chars == 0) {
        printf("An empty name was put in. Aborting\n");
        return -2;
    }

    buffer[chars] = 0;
    printf("Nice to meet you %s!\n", buffer);

    return 0;
}