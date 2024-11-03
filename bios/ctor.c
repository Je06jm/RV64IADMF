#include "ctor.h"

int ctor_init() {
    if (!ctor_setup_stdlib())
        return 0;
    
    if (!ctor_setup_stdio())
        return 0;
    
    return 1;
}

void ctor_cleanup() {
    ctor_cleanup_stdio();
    ctor_cleanup_stdlib();
}