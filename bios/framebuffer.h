#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

void framebuffer_setup();

void framebuffer_getsize(uint32_t* width, uint32_t* height);

void framebuffer_getclearcolor(uint8_t* r, uint8_t* g, uint8_t* b);
void framebuffer_setclearcolor(uint8_t r, uint8_t g, uint8_t b);
void framebuffer_clear();

void framebuffer_plot(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);

#endif