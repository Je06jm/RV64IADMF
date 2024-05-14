#ifndef FLOAT_HPP
#define FLOAT_HPP

#include "Types.hpp"

union Float {
    double d;
    Long u64;
    struct {
        union {
            float f;
            Word u32;
        };
        Word is_double;
    };
};

#endif