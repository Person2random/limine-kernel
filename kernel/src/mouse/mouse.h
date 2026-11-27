#ifndef MOUSE_H
#define MOUSE_H

#include <stdbool.h>
#include <stdint.h>

struct mouse_event {
    int8_t dx;
    int8_t dy;
    uint8_t buttons;
};

bool ps2_mouse_init(void);
bool mouse_read_event(struct mouse_event *event);
void mouse_cursor_erase(void);
void mouse_cursor_draw(uint32_t x, uint32_t y);

#endif
