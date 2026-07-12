#include <stdint.h>
#include <stdint.h>
#ifndef GRAPHICS_H
#define GRAPHICS_H
void graphics_init(void);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
extern uint64_t width;
extern uint64_t height;
#endif
