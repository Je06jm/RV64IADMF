#ifndef FLOAT_HPP
#define FLOAT_HPP

#include <stdint.h>

union Float {
    double d;
    uint64_t u64;
    struct {
        union {
            float f;
            uint32_t u32;
        };
        uint32_t is_double;
    };
};

#endif