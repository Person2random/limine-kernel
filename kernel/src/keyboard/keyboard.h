#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct keyboard_event {
    uint8_t scancode;
    char character;
    bool extended;
    bool pressed;
};

bool ps2_keyboard_init(void);
bool keyboard_read_event(struct keyboard_event *event);
bool keyboard_take_app_exit(void);
size_t ps2_keyboard_dropped_events(void);

#endif
