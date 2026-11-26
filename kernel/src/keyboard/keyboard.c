#include "keyboard.h"

extern uint16_t ps2_keyboard_read_event(void);

static bool shift_down;
static bool caps_lock;
static bool left_control_down;
static bool right_control_down;
static bool app_exit_requested;

static char translate_scancode(uint8_t scancode, bool shifted) {
    static const char plain[] =
        "\0\0331234567890-=\b\t"
        "qwertyuiop[]\n\0"
        "asdfghjkl;'\x60"
        "\0\\zxcvbnm,./";
    static const char shifted_chars[] =
        "\0\033!@#$%^&*()_+\b\t"
        "QWERTYUIOP{}\n\0"
        "ASDFGHJKL:\"\x7E"
        "\0|ZXCVBNM<>?";

    if (scancode == 0x39) {
        return ' ';
    }
    if (scancode >= sizeof(plain)) {
        return '\0';
    }

    char character = shifted ? shifted_chars[scancode] : plain[scancode];
    if (character >= 'a' && character <= 'z' && caps_lock) {
        character -= 'a' - 'A';
    } else if (character >= 'A' && character <= 'Z' && caps_lock) {
        character += 'a' - 'A';
    }
    return character;
}

bool keyboard_read_event(struct keyboard_event *event) {
    uint16_t raw = ps2_keyboard_read_event();
    if (raw == 0) {
        return false;
    }

    event->scancode = raw & 0x7F;
    event->extended = (raw & 0x0100) != 0;
    event->pressed = (raw & 0x8000) != 0;
    event->character = '\0';

    if (!event->extended) {
        if (event->scancode == 0x2A || event->scancode == 0x36) {
            shift_down = event->pressed;
        } else if (event->scancode == 0x3A && event->pressed) {
            caps_lock = !caps_lock;
        } else if (event->scancode == 0x1D) {
            left_control_down = event->pressed;
        }
    } else if (event->scancode == 0x1D) {
        right_control_down = event->pressed;
    }

    if (event->pressed && event->extended == false &&
        event->scancode == 0x2E &&
        (left_control_down || right_control_down)) {
        app_exit_requested = true;
    }

    if (event->pressed && !event->extended) {
        bool shifted = shift_down != caps_lock;
        event->character = translate_scancode(event->scancode, shifted);
    }
    return true;
}

bool keyboard_take_app_exit(void) {
    bool requested = app_exit_requested;
    app_exit_requested = false;
    return requested;
}
