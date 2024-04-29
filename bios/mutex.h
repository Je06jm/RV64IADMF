#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>

typedef uint32_t Mutex;

void mutex_lock(Mutex* mutex);
void mutex_unlock(Mutex* mutex);

#endif