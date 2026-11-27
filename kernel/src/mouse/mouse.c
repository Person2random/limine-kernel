#include "mouse.h"
#include "../graphics/graphics.h"

extern uint32_t ps2_mouse_read_event(void);
extern uint32_t graphics_get_pixel(uint32_t x, uint32_t y);

static bool cursor_visible;
static uint32_t cursor_x;
static uint32_t cursor_y;
static uint32_t cursor_underlay[8][8];

bool mouse_read_event(struct mouse_event *event) {
    uint32_t raw = ps2_mouse_read_event();
    if ((raw & 0x80000000) == 0) {
        return false;
    }
    event->dx = (int8_t)(raw >> 8);
    event->dy = (int8_t)raw;
    event->buttons = (raw >> 16) & 0x07;
    return true;
}

void mouse_cursor_erase(void) {
    if (!cursor_visible) {
        return;
    }
    for (uint32_t y = 0; y < 8; y++) {
        for (uint32_t x = 0; x < 8; x++) {
            if (cursor_x + x < width && cursor_y + y < height) {
                put_pixel(cursor_x + x, cursor_y + y, cursor_underlay[y][x]);
            }
        }
    }
    cursor_visible = false;
}

void mouse_cursor_draw(uint32_t x, uint32_t y) {
    static const char *const shape[8] = {
        "X.......", "XX......", "X#X.....", "X##X....",
        "X###X...", "X####X..", "X#####X.", "X##....."
    };
    mouse_cursor_erase();
    cursor_x = x;
    cursor_y = y;
    for (uint32_t row = 0; row < 8; row++) {
        for (uint32_t column = 0; column < 8; column++) {
            uint32_t pixel_x = x + column;
            uint32_t pixel_y = y + row;
            if (pixel_x >= width || pixel_y >= height) {
                cursor_underlay[row][column] = 0;
                continue;
            }
            cursor_underlay[row][column] =
                graphics_get_pixel(pixel_x, pixel_y);
            if (shape[row][column] == 'X') {
                put_pixel(pixel_x, pixel_y, 0xFFFFFF);
            } else if (shape[row][column] == '#') {
                put_pixel(pixel_x, pixel_y, 0x000000);
            }
        }
    }
    cursor_visible = true;
}
