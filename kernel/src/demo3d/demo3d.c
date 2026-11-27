#include "demo3d.h"
#include "../devices/tsc/tsc.h"
#include "../graphics/graphics.h"
#include "../keyboard/keyboard.h"
#include "../terminal/terminal.h"
#include <stdbool.h>
#include <stdint.h>

#define COLOR_BACKGROUND 0x101820
#define COLOR_CUBE 0x7CFC9A
#define COLOR_TEXT 0xFFFFFF
#define COLOR_ACCENT 0x36C9E8
#define VERTEX_COUNT 8
#define EDGE_COUNT 12
#define Q 1000

static const int32_t vertices[VERTEX_COUNT][3] = {
    {-100, -100, -100}, {100, -100, -100},
    {100, 100, -100}, {-100, 100, -100},
    {-100, -100, 100}, {100, -100, 100},
    {100, 100, 100}, {-100, 100, 100}
};

static const uint8_t edges[EDGE_COUNT][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

static int32_t sine_degrees(int32_t degrees) {
    // Integer trig keeps the floating-point unit out of this kernel demo.
    degrees %= 360;
    if (degrees < 0) {
        degrees += 360;
    }
    int32_t sign = 1;
    if (degrees > 180) {
        degrees = 360 - degrees;
        sign = -1;
    }
    int32_t product = degrees * (180 - degrees);
    return sign * (4 * product * Q) / (40500 - product);
}

static int32_t cosine_degrees(int32_t degrees) {
    return sine_degrees(degrees + 90);
}

static void draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                      uint32_t color) {
    int32_t dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    int32_t sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 >= y0 ? y0 - y1 : y1 - y0;
    int32_t sy = y0 < y1 ? 1 : -1;
    int32_t error = dx + dy;

    for (;;) {
        if (x0 >= 0 && y0 >= 0 && (uint64_t)x0 < width &&
            (uint64_t)y0 < height) {
            put_pixel((uint32_t)x0, (uint32_t)y0, color);
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int32_t twice_error = 2 * error;
        if (twice_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

static void draw_cube(int32_t angle, uint32_t color) {
    int32_t projected_x[VERTEX_COUNT];
    int32_t projected_y[VERTEX_COUNT];
    int32_t sin_x = sine_degrees(angle);
    int32_t cos_x = cosine_degrees(angle);
    int32_t sin_y = sine_degrees(angle * 2);
    int32_t cos_y = cosine_degrees(angle * 2);
    int32_t sin_z = sine_degrees(angle / 2);
    int32_t cos_z = cosine_degrees(angle / 2);
    int32_t center_x = (int32_t)(width / 2);
    int32_t center_y = (int32_t)(height / 2);
    int32_t focal_length = center_x < center_y ? center_x : center_y;
    if (focal_length > 0) {
        focal_length = focal_length * 3 / 2;
    }

    for (uint32_t i = 0; i < VERTEX_COUNT; i++) {
        int32_t x = vertices[i][0];
        int32_t y = vertices[i][1];
        int32_t z = vertices[i][2];
        int32_t rotated_y = (y * cos_x - z * sin_x) / Q;
        int32_t rotated_z = (y * sin_x + z * cos_x) / Q;
        int32_t rotated_x = (x * cos_y + rotated_z * sin_y) / Q;
        rotated_z = (-x * sin_y + rotated_z * cos_y) / Q;
        int32_t final_x = (rotated_x * cos_z - rotated_y * sin_z) / Q;
        int32_t final_y = (rotated_x * sin_z + rotated_y * cos_z) / Q;
        int32_t depth = rotated_z + 420;
        projected_x[i] = center_x + final_x * focal_length / depth;
        projected_y[i] = center_y - final_y * focal_length / depth;
    }
    for (uint32_t edge = 0; edge < EDGE_COUNT; edge++) {
        draw_line(projected_x[edges[edge][0]], projected_y[edges[edge][0]],
                  projected_x[edges[edge][1]], projected_y[edges[edge][1]],
                  color);
    }
}

void demo3d_run(void) {
    if (width < 320 || height < 240 || width > INT32_MAX ||
        height > INT32_MAX) {
        print("Framebuffer is too small for 3D. The cube is very sorry.\n");
        return;
    }

    uint64_t frame_ticks = tsc_get_freq() / 30;
    if (frame_ticks == 0) {
        print("The cube needs a working TSC timer.\n");
        return;
    }
    for (uint32_t y = 0; y < (uint32_t)height; y++) {
        for (uint32_t x = 0; x < (uint32_t)width; x++) {
            put_pixel(x, y, COLOR_BACKGROUND);
        }
    }
    terminal_init(false);
    print("3D DEMO - rotating wireframe cube | Ctrl+C exits\n");

    int32_t angle = 0;
    uint64_t next_frame = rdtsc() + frame_ticks;
    draw_cube(angle, COLOR_CUBE);
    for (;;) {
        struct keyboard_event event;
        while (keyboard_read_event(&event)) {
            if (keyboard_take_app_exit()) {
                return;
            }
        }
        if (rdtsc() < next_frame) {
            __asm__ volatile("pause");
            continue;
        }
        next_frame = rdtsc() + frame_ticks;
        draw_cube(angle, COLOR_BACKGROUND);
        angle = (angle + 3) % 360;
        draw_cube(angle, COLOR_CUBE);
    }
}
