#include "text.h"

#include "framebuffer.h"
#include "printf.h"

const char text_bits[] = {
    0b00111100,
    0b01000010,
    0b11000011,
    0b10000001,
    0b10000001,
    0b11111111,
    0b10000001,
    0b10000001,
};

void text_plot(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    for (int sy = y, by = 0; by < 8; sy++, by++) {
        for (int sx = x, val = text_bits[by]; val != 0; sx++, val >>= 1) {
            if (val & 1)
                framebuffer_plot(sx, sy, r, g, b);
        }
    }
}