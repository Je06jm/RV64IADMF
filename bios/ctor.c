#include "ctor.h"

#include "machine.h"

int ctor_init() {
    if (!ctor_setup_stdlib())
        return false;
    
    if (!ctor_setup_stdio())
        return false;
    
    return true;
}

void ctor_cleanup() {
    ctor_cleanup_stdio();
    ctor_cleanup_stdlib();
}