#include "framebuffer.h"

#include "machine.h"

uint32_t framebuffer_width;
uint32_t framebuffer_height;
uint32_t framebuffer_address;

typedef union {
    struct {
        uint8_t r, g, b, _unused;
    };
    uint32_t word;
} FramebufferPixel;

FramebufferPixel framebuffer_clearcolor;

void framebuffer_setup() {
    machine_call(MACHINE_CALL_GET_SCREEN_SIZE, &framebuffer_width, &framebuffer_height);
    framebuffer_address = machine_call(MACHINE_CALL_GET_SCREEN_ADDRESS);

    framebuffer_clearcolor.r = 0;
    framebuffer_clearcolor.g = 0;
    framebuffer_clearcolor.b = 0;
}

void framebuffer_getsize(uint32_t* width, uint32_t* height) {
    if (width) *width = framebuffer_width;
    if (height) *height = framebuffer_height;
}

void framebuffer_getclearcolor(uint8_t* r, uint8_t* g, uint8_t* b) {
    if (r) *r = framebuffer_clearcolor.r;
    if (g) *g = framebuffer_clearcolor.g;
    if (b) *b = framebuffer_clearcolor.b;
}

void framebuffer_setclearcolor(uint8_t r, uint8_t g, uint8_t b) {
    framebuffer_clearcolor.r = r;
    framebuffer_clearcolor.g = g;
    framebuffer_clearcolor.b = b;
}

void framebuffer_clear() {
    FramebufferPixel* pixels = (FramebufferPixel*)framebuffer_address;
    for (uint32_t i = 0; i < (framebuffer_width * framebuffer_height); i++)
        pixels[i].word = framebuffer_clearcolor.word;
}

void framebuffer_plot(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= framebuffer_width) return;
    if (y >= framebuffer_height) return;

    uint32_t index = (y * framebuffer_width) + x;
    FramebufferPixel* pixels = (FramebufferPixel*)framebuffer_address;
    pixels[index].r = r;
    pixels[index].g = g;
    pixels[index].b = b;
}