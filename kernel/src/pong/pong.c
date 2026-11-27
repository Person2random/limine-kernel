#include "pong.h"
#include "../devices/tsc/tsc.h"
#include "../graphics/graphics.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../terminal/terminal.h"
#include <stdbool.h>
#include <stdint.h>

#define COLOR_BACKGROUND 0x101820
#define COLOR_TEXT 0xFFFFFF
#define COLOR_ACCENT 0x7CFC9A
#define COLOR_BALL 0xFF5C5C
#define PADDLE_WIDTH 12
#define PADDLE_HEIGHT 112
#define BALL_SIZE 14
#define PADDLE_SPEED 9
#define PAUSE_BUTTON_WIDTH 112
#define PAUSE_BUTTON_HEIGHT 32

static void fill_rect(int32_t x, int32_t y, uint32_t w, uint32_t h,
                      uint32_t color) {
    for (uint32_t row = 0; row < h; row++) {
        for (uint32_t column = 0; column < w; column++) {
            put_pixel((uint32_t)(x + (int32_t)column),
                      (uint32_t)(y + (int32_t)row), color);
        }
    }
}

static void draw_score(uint32_t left, uint32_t right) {
    uint32_t center = (uint32_t)(width / 16);
    put_char(center - 2, 2, '0' + left % 10, COLOR_TEXT, COLOR_BACKGROUND);
    put_char(center, 2, ':', COLOR_TEXT, COLOR_BACKGROUND);
    put_char(center + 2, 2, '0' + right % 10, COLOR_TEXT, COLOR_BACKGROUND);
}

static void draw_paddle(int32_t x, int32_t y, uint32_t color) {
    fill_rect(x, y, PADDLE_WIDTH, PADDLE_HEIGHT, color);
}

static int32_t pause_button_x(void) {
    return (int32_t)width - PAUSE_BUTTON_WIDTH - 12;
}

static void draw_pause_button(bool paused) {
    uint32_t background = paused ? COLOR_ACCENT : 0x304050;
    int32_t x = pause_button_x();
    fill_rect(x, 8, PAUSE_BUTTON_WIDTH, PAUSE_BUTTON_HEIGHT, background);
    const char *label = paused ? "RESUME" : "PAUSE";
    for (uint32_t i = 0; label[i]; i++) {
        put_char((uint32_t)(x / 8 + 4) + i, 1, label[i], COLOR_TEXT,
                 background);
    }
}

static void draw_scene(int32_t left_x, int32_t left_y, int32_t right_x,
                       int32_t right_y, int32_t ball_x, int32_t ball_y,
                       uint32_t left_score, uint32_t right_score,
                       bool paused) {
    fill_rect(0, 0, (uint32_t)width, (uint32_t)height, COLOR_BACKGROUND);
    for (uint32_t y = 16; y < (uint32_t)height; y += 32) {
        fill_rect((int32_t)width / 2 - 2, (int32_t)y, 4, 16, 0x304050);
    }
    draw_score(left_score, right_score);
    draw_paddle(left_x, left_y, COLOR_ACCENT);
    draw_paddle(right_x, right_y, COLOR_ACCENT);
    fill_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, COLOR_BALL);
    draw_pause_button(paused);
    if (paused) {
        fill_rect((int32_t)width / 2 - 100, (int32_t)height / 2 - 28,
                  200, 56, 0x304050);
        const char label[] = "PAUSED";
        for (uint32_t i = 0; label[i]; i++) {
            put_char((uint32_t)(width / 16) - 3 + i,
                     (uint32_t)(height / 32), label[i], COLOR_TEXT, 0x304050);
        }
    }
}

void pong_run(void) {
    if (width < 400 || height < 240 || width > INT32_MAX ||
        height > INT32_MAX) {
        print("Framebuffer is too small to play Pong.\n");
        return;
    }

    int32_t screen_width = (int32_t)width;
    int32_t screen_height = (int32_t)height;
    int32_t left_x = 32;
    int32_t right_x = screen_width - 32 - PADDLE_WIDTH;
    int32_t left_y = (screen_height - PADDLE_HEIGHT) / 2;
    int32_t right_y = left_y;
    int32_t ball_x = screen_width / 2;
    int32_t ball_y = screen_height / 2;
    int32_t ball_dx = 7;
    int32_t ball_dy = 4;
    uint32_t left_score = 0;
    uint32_t right_score = 0;
    int32_t old_left_y = left_y;
    int32_t old_right_y = right_y;
    int32_t old_ball_x = ball_x;
    int32_t old_ball_y = ball_y;
    uint64_t frame_ticks = tsc_get_freq() / 60;

    if (frame_ticks == 0) {
        print("Pong needs a working TSC timer.\n");
        return;
    }

    fill_rect(0, 0, (uint32_t)screen_width, (uint32_t)screen_height,
              COLOR_BACKGROUND);
    terminal_init(false);
    print("PONG - W/S vs arrows | P or click Pause | Ctrl+C exits\n");
    bool paused = false;
    int32_t mouse_x = screen_width / 2;
    int32_t mouse_y = screen_height / 2;
    uint8_t previous_buttons = 0;
    draw_scene(left_x, left_y, right_x, right_y, ball_x, ball_y,
               left_score, right_score, paused);
    mouse_cursor_draw((uint32_t)mouse_x, (uint32_t)mouse_y);
    uint64_t next_frame = rdtsc() + frame_ticks;

    for (;;) {
        mouse_cursor_erase();
        bool redraw = false;
        struct keyboard_event event;
        while (keyboard_read_event(&event)) {
            if (keyboard_take_app_exit()) {
                mouse_cursor_erase();
                return;
            }
            if (!event.pressed) {
                continue;
            }
            if (!event.extended && event.scancode == 0x19) {
                paused = !paused;
                redraw = true;
                continue;
            }
            if (paused) {
                continue;
            }
            if (event.extended && event.scancode == 0x48 && right_y > 8) {
                right_y -= PADDLE_SPEED;
            } else if (event.extended && event.scancode == 0x50 &&
                       right_y + PADDLE_HEIGHT < screen_height - 8) {
                right_y += PADDLE_SPEED;
            } else if (!event.extended && event.scancode == 0x11 &&
                       left_y > 8) {
                left_y -= PADDLE_SPEED;
            } else if (!event.extended && event.scancode == 0x1F &&
                       left_y + PADDLE_HEIGHT < screen_height - 8) {
                left_y += PADDLE_SPEED;
            }
        }

        struct mouse_event pointer_event;
        while (mouse_read_event(&pointer_event)) {
            mouse_x += pointer_event.dx;
            mouse_y -= pointer_event.dy;
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x >= screen_width) mouse_x = screen_width - 1;
            if (mouse_y >= screen_height) mouse_y = screen_height - 1;
            bool clicked = (pointer_event.buttons & 1) != 0 &&
                           (previous_buttons & 1) == 0;
            previous_buttons = pointer_event.buttons;
            if (clicked && mouse_x >= pause_button_x() &&
                mouse_x < screen_width - 12 && mouse_y >= 8 &&
                mouse_y < 8 + PAUSE_BUTTON_HEIGHT) {
                paused = !paused;
                redraw = true;
            }
        }
        if (redraw) {
            draw_scene(left_x, left_y, right_x, right_y, ball_x, ball_y,
                       left_score, right_score, paused);
        }

        if (rdtsc() < next_frame) {
            mouse_cursor_draw((uint32_t)mouse_x, (uint32_t)mouse_y);
            __asm__ volatile("pause");
            continue;
        }
        next_frame = rdtsc() + frame_ticks;

        if (paused) {
            mouse_cursor_draw((uint32_t)mouse_x, (uint32_t)mouse_y);
            __asm__ volatile("pause");
            continue;
        }

        draw_paddle(left_x, old_left_y, COLOR_BACKGROUND);
        draw_paddle(right_x, old_right_y, COLOR_BACKGROUND);
        fill_rect(old_ball_x, old_ball_y, BALL_SIZE, BALL_SIZE,
                  COLOR_BACKGROUND);

        ball_x += ball_dx;
        ball_y += ball_dy;
        if (ball_y <= 8 || ball_y + BALL_SIZE >= screen_height - 8) {
            ball_dy = -ball_dy;
            ball_y += ball_dy;
        }
        // A paddle hit adds English; physics has kindly waived the paperwork.
        if (ball_dx < 0 && ball_x <= left_x + PADDLE_WIDTH &&
            ball_x + BALL_SIZE >= left_x && ball_y + BALL_SIZE >= left_y &&
            ball_y <= left_y + PADDLE_HEIGHT) {
            ball_dx = -ball_dx;
            ball_dy += (ball_y + BALL_SIZE / 2 -
                        (left_y + PADDLE_HEIGHT / 2)) / 12;
            if (ball_dy > 12) ball_dy = 12;
            if (ball_dy < -12) ball_dy = -12;
            ball_x = left_x + PADDLE_WIDTH + 1;
        } else if (ball_dx > 0 &&
                   ball_x + BALL_SIZE >= right_x &&
                   ball_x <= right_x + PADDLE_WIDTH &&
                   ball_y + BALL_SIZE >= right_y &&
                   ball_y <= right_y + PADDLE_HEIGHT) {
            ball_dx = -ball_dx;
            ball_dy += (ball_y + BALL_SIZE / 2 -
                        (right_y + PADDLE_HEIGHT / 2)) / 12;
            if (ball_dy > 12) ball_dy = 12;
            if (ball_dy < -12) ball_dy = -12;
            ball_x = right_x - BALL_SIZE - 1;
        }

        if (ball_x + BALL_SIZE < 0 || ball_x > screen_width) {
            if (ball_x < 0) {
                right_score++;
                ball_dx = 7;
            } else {
                left_score++;
                ball_dx = -7;
            }
            ball_x = screen_width / 2;
            ball_y = screen_height / 2;
            ball_dy = (left_score + right_score) % 2 ? 4 : -4;
            draw_score(left_score, right_score);
        }

        draw_paddle(left_x, left_y, COLOR_ACCENT);
        draw_paddle(right_x, right_y, COLOR_ACCENT);
        fill_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, COLOR_BALL);
        draw_pause_button(paused);
        old_left_y = left_y;
        old_right_y = right_y;
        old_ball_x = ball_x;
        old_ball_y = ball_y;
        mouse_cursor_draw((uint32_t)mouse_x, (uint32_t)mouse_y);
    }
}
