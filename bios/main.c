#include "printf.h"
#include "input.h"
#include "phys_memory.h"
#include "framebuffer.h"
#include "text.h"
#include "cpu.h"
#include "machine.h"
#include "mutex.h"

#define NULL ((void*)0)

void bootstrap_start();

int main(int argc, const char** argv) {
    //phys_setup();

    framebuffer_setup();
    framebuffer_setclearcolor(255, 255, 255);
    framebuffer_clear();

    uint32_t harts[32];
    uint32_t cpus = get_cpus(harts);

    for (uint32_t i = 0; i < cpus; i++)
        if (i != current_hart())
            start_cpu(i, bootstrap_start, 0x40000 + 0x2000 * (i - 1));

    bootstrap_start();

    return 0;
}

int counter = 0;
Mutex mutex = 0;

void bootstrap_start() {
    //printf("I'm %u\n", current_hart());
    
    int i = 0;
    while (1) {
        int height;
        if (i > 100)
            height = 200 - i;
        
        else
            height = i;

        text_plot(10 * current_hart() + 10, 10 + height, 0, 0, 0);

        for (int j = 0; j < 10000; j++) {}

        text_plot(10 * current_hart() + 10, 10 + height, 255, 255, 255);

        i += 1;
        i %= 200;
    }

    while (1) {}
}