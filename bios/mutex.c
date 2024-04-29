#include "mutex.h"

#include "cpu.h"

#include <stdbool.h>

uint32_t load_reserve(uint32_t address);
bool store_conditional(uint32_t address, uint32_t value);

void mutex_lock(Mutex* mutex) {
    uint32_t hart = current_hart();
    uint32_t address = (uint32_t)mutex;

    while (true) {
        uint32_t value = load_reserve(address);
        if (value) continue;

        if (store_conditional(address, 1)) {
            for (uint32_t i = 0; i < hart; i++) {}
        }
        else break;
    }
}

void mutex_unlock(Mutex* mutex) {
    *mutex = 0;
}