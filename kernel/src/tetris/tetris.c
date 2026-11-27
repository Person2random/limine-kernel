#include "tetris.h"
#include "../devices/tsc/tsc.h"
#include "../graphics/graphics.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../terminal/terminal.h"
#include <stdbool.h>
#include <stdint.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define COLOR_BACKGROUND 0x101820
#define COLOR_BOARD 0x182830
#define COLOR_TEXT 0xFFFFFF
#define COLOR_ACCENT 0x7CFC9A
#define PAUSE_BUTTON_WIDTH 112
#define PAUSE_BUTTON_HEIGHT 32

static const uint8_t piece_blocks[7][4][2] = {
    {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
    {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
    {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
    {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
    {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
    {{2, 0}, {0, 1}, {1, 1}, {2, 1}}
};

static const uint32_t piece_colors[7] = {
    0x36C9E8, 0xF5D547, 0xB45AF2, 0x55D68B,
    0xE85959, 0x528BFF, 0xF09A45
};

static uint8_t board[BOARD_HEIGHT][BOARD_WIDTH];

struct piece {
    uint32_t type;
    int32_t x;
    int32_t y;
    uint8_t rotation;
};

static int32_t pause_button_x(void) {
    return (int32_t)width - PAUSE_BUTTON_WIDTH - 12;
}

static void draw_pause_button(bool paused) {
    uint32_t background = paused ? COLOR_ACCENT : 0x304050;
    int32_t x = pause_button_x();
    for (uint32_t y = 8; y < 8 + PAUSE_BUTTON_HEIGHT; y++) {
        for (uint32_t column = 0; column < PAUSE_BUTTON_WIDTH; column++) {
            put_pixel((uint32_t)x + column, y, background);
        }
    }
    const char *label = paused ? "RESUME" : "PAUSE";
    for (uint32_t i = 0; label[i]; i++) {
        put_char((uint32_t)(x / 8 + 4) + i, 1, label[i], COLOR_TEXT,
                 background);
    }
}

static void draw_pause_overlay(void) {
    int32_t x = (int32_t)width / 2 - 100;
    int32_t y = (int32_t)height / 2 - 28;
    for (uint32_t row = 0; row < 56; row++) {
        for (uint32_t column = 0; column < 200; column++) {
            put_pixel((uint32_t)(x + (int32_t)column),
                      (uint32_t)(y + (int32_t)row), 0x304050);
        }
    }
    const char label[] = "PAUSED";
    for (uint32_t i = 0; label[i]; i++) {
        put_char((uint32_t)(width / 16) - 3 + i,
                 (uint32_t)(height / 32), label[i], COLOR_TEXT, 0x304050);
    }
}

static uint32_t random_piece(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return (uint32_t)(*state % 7);
}

static void block_position(const struct piece *piece, uint32_t block,
                           int32_t *x, int32_t *y) {
    int32_t bx = piece_blocks[piece->type][block][0];
    int32_t by = piece_blocks[piece->type][block][1];
    for (uint8_t turn = 0; turn < piece->rotation; turn++) {
        int32_t next_x = 3 - by;
        by = bx;
        bx = next_x;
    }
    *x = piece->x + bx;
    *y = piece->y + by;
}

static bool fits(const struct piece *piece) {
    for (uint32_t block = 0; block < 4; block++) {
        int32_t x;
        int32_t y;
        block_position(piece, block, &x, &y);
        if (x < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT) {
            return false;
        }
        if (y >= 0 && board[y][x] != 0) {
            return false;
        }
    }
    return true;
}

static void draw_block(uint32_t x, uint32_t y, uint32_t cell_size,
                       uint32_t color) {
    uint32_t left = (uint32_t)(width - (BOARD_WIDTH * cell_size)) / 2 +
                    x * cell_size;
    uint32_t top = 56 + y * cell_size;
    for (uint32_t row = 0; row < cell_size - 1; row++) {
        for (uint32_t column = 0; column < cell_size - 1; column++) {
            put_pixel(left + column, top + row, color);
        }
    }
}

static void draw_game(const struct piece *current, uint32_t cell_size,
                      uint32_t score, bool game_over, bool paused) {
    uint32_t board_left = (uint32_t)(width - (BOARD_WIDTH * cell_size)) / 2;
    for (uint32_t row = 0; row < BOARD_HEIGHT; row++) {
        for (uint32_t column = 0; column < BOARD_WIDTH; column++) {
            draw_block(column, row, cell_size,
                       board[row][column]
                           ? piece_colors[board[row][column] - 1]
                           : COLOR_BOARD);
        }
    }
    for (uint32_t block = 0; block < 4; block++) {
        int32_t x;
        int32_t y;
        block_position(current, block, &x, &y);
        if (y >= 0 && y < BOARD_HEIGHT) {
            draw_block((uint32_t)x, (uint32_t)y, cell_size,
                       piece_colors[current->type]);
        }
    }
    put_char(board_left / 8, 2, 'S', COLOR_ACCENT, COLOR_BACKGROUND);
    put_char(board_left / 8 + 1, 2, 'C', COLOR_ACCENT, COLOR_BACKGROUND);
    put_char(board_left / 8 + 2, 2, 'O', COLOR_ACCENT, COLOR_BACKGROUND);
    put_char(board_left / 8 + 3, 2, 'R', COLOR_ACCENT, COLOR_BACKGROUND);
    put_char(board_left / 8 + 4, 2, 'E', COLOR_ACCENT, COLOR_BACKGROUND);
    put_char(board_left / 8 + 5, 2, ':', COLOR_ACCENT, COLOR_BACKGROUND);
    put_char(board_left / 8 + 7, 2, '0' + (score / 100) % 10,
             COLOR_TEXT, COLOR_BACKGROUND);
    put_char(board_left / 8 + 8, 2, '0' + (score / 10) % 10,
             COLOR_TEXT, COLOR_BACKGROUND);
    put_char(board_left / 8 + 9, 2, '0' + score % 10,
             COLOR_TEXT, COLOR_BACKGROUND);
    if (game_over) {
        put_char(board_left / 8, 3, 'G', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 1, 3, 'A', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 2, 3, 'M', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 3, 3, 'E', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 5, 3, 'O', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 6, 3, 'V', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 7, 3, 'E', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 8, 3, 'R', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8, 4, 'R', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 1, 4, ' ', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 2, 4, 'T', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 3, 4, 'O', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 4, 4, ' ', COLOR_TEXT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 5, 4, 'R', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 6, 4, 'E', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 7, 4, 'S', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 8, 4, 'T', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 9, 4, 'A', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 10, 4, 'R', COLOR_ACCENT, COLOR_BACKGROUND);
        put_char(board_left / 8 + 11, 4, 'T', COLOR_ACCENT, COLOR_BACKGROUND);
    }
    draw_pause_button(paused);
}

static bool lock_piece(const struct piece *piece) {
    for (uint32_t block = 0; block < 4; block++) {
        int32_t x;
        int32_t y;
        block_position(piece, block, &x, &y);
        if (y < 0) {
            return false;
        }
        board[y][x] = (uint8_t)(piece->type + 1);
    }
    return true;
}

static uint32_t clear_full_rows(void) {
    uint32_t cleared = 0;
    int32_t row = BOARD_HEIGHT - 1;
    while (row >= 0) {
        bool full = true;
        for (uint32_t column = 0; column < BOARD_WIDTH; column++) {
            if (board[row][column] == 0) {
                full = false;
                break;
            }
        }
        if (!full) {
            row--;
            continue;
        }
        // Pull the rows down one at a time; gravity finally has a useful job.
        for (int32_t move_row = row; move_row > 0; move_row--) {
            for (uint32_t column = 0; column < BOARD_WIDTH; column++) {
                board[move_row][column] = board[move_row - 1][column];
            }
        }
        for (uint32_t column = 0; column < BOARD_WIDTH; column++) {
            board[0][column] = 0;
        }
        cleared++;
    }
    return cleared;
}

void tetris_run(void) {
    if (width < 240 || height < 420 || width > UINT32_MAX ||
        height > UINT32_MAX) {
        print("Framebuffer is too small to play Tetris.\n");
        return;
    }
    uint32_t cell_size = (uint32_t)(height - 72) / BOARD_HEIGHT;
    if (cell_size > (uint32_t)width / (BOARD_WIDTH + 2)) {
        cell_size = (uint32_t)width / (BOARD_WIDTH + 2);
    }
    if (cell_size < 8) {
        print("Framebuffer is too small to play Tetris.\n");
        return;
    }

    for (uint32_t row = 0; row < BOARD_HEIGHT; row++) {
        for (uint32_t column = 0; column < BOARD_WIDTH; column++) {
            board[row][column] = 0;
        }
    }
    uint64_t random_state = rdtsc() | 1;
    struct piece current = {.type = random_piece(&random_state), .x = 3, .y = 0};
    uint32_t score = 0;
    bool game_over = false;
    uint64_t gravity_ticks = tsc_get_freq() / 2;
    if (gravity_ticks == 0) {
        print("Tetris needs a working TSC timer.\n");
        return;
    }

    for (uint32_t y = 0; y < (uint32_t)height; y++) {
        for (uint32_t x = 0; x < (uint32_t)width; x++) {
            put_pixel(x, y, COLOR_BACKGROUND);
        }
    }
    terminal_init(false);
    print("TETRIS - arrows, Z/X, Space | P or click Pause | Ctrl+C exits\n");
    uint64_t next_gravity = rdtsc() + gravity_ticks;
    bool paused = false;
    int32_t mouse_x = (int32_t)width / 2;
    int32_t mouse_y = (int32_t)height / 2;
    uint8_t previous_buttons = 0;
    draw_game(&current, cell_size, score, game_over, paused);
    mouse_cursor_draw((uint32_t)mouse_x, (uint32_t)mouse_y);

    for (;;) {
        mouse_cursor_erase();
        struct keyboard_event event;
        bool changed = false;
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
                if (!paused) {
                    next_gravity = rdtsc() + gravity_ticks;
                }
                changed = true;
                continue;
            }
            if (paused) {
                continue;
            }
            if (game_over &&
                (event.extended || event.scancode != 0x13)) {
                continue;
            }
            if (event.extended) {
                struct piece moved = current;
                if (event.scancode == 0x4B) {
                    moved.x--;
                } else if (event.scancode == 0x4D) {
                    moved.x++;
                } else if (event.scancode == 0x50) {
                    moved.y++;
                    if (fits(&moved)) {
                        current = moved;
                        score++;
                    }
                } else {
                    continue;
                }
                if (fits(&moved)) {
                    current = moved;
                }
                changed = true;
            } else if (event.scancode == 0x2C || event.scancode == 0x2D) {
                struct piece rotated = current;
                rotated.rotation = (uint8_t)((rotated.rotation +
                                      (event.scancode == 0x2D ? 1 : 3)) % 4);
                if (fits(&rotated)) {
                    current = rotated;
                    changed = true;
                }
            } else if (event.scancode == 0x39 && !game_over) {
                struct piece dropped = current;
                while (fits(&dropped)) {
                    current = dropped;
                    dropped.y++;
                }
                changed = true;
            } else if (event.scancode == 0x13 && game_over) {
                for (uint32_t row = 0; row < BOARD_HEIGHT; row++) {
                    for (uint32_t column = 0; column < BOARD_WIDTH; column++) {
                        board[row][column] = 0;
                    }
                }
                score = 0;
                game_over = false;
                current = (struct piece){
                    .type = random_piece(&random_state), .x = 3, .y = 0
                };
                next_gravity = rdtsc() + gravity_ticks;
                changed = true;
            }
        }

        struct mouse_event pointer_event;
        while (mouse_read_event(&pointer_event)) {
            mouse_x += pointer_event.dx;
            mouse_y -= pointer_event.dy;
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if ((uint64_t)mouse_x >= width) mouse_x = (int32_t)width - 1;
            if ((uint64_t)mouse_y >= height) mouse_y = (int32_t)height - 1;
            bool clicked = (pointer_event.buttons & 1) != 0 &&
                           (previous_buttons & 1) == 0;
            previous_buttons = pointer_event.buttons;
            if (clicked && mouse_x >= pause_button_x() &&
                mouse_x < (int32_t)width - 12 && mouse_y >= 8 &&
                mouse_y < 8 + PAUSE_BUTTON_HEIGHT) {
                paused = !paused;
                if (!paused) {
                    next_gravity = rdtsc() + gravity_ticks;
                }
                changed = true;
            }
        }

        if (!paused && !game_over && rdtsc() >= next_gravity) {
            next_gravity = rdtsc() + gravity_ticks;
            struct piece fallen = current;
            fallen.y++;
            if (fits(&fallen)) {
                current = fallen;
            } else if (!lock_piece(&current)) {
                game_over = true;
            } else {
                uint32_t rows_cleared = clear_full_rows();
                score += rows_cleared * rows_cleared * 100;
                current = (struct piece){
                    .type = random_piece(&random_state), .x = 3, .y = 0
                };
                if (!fits(&current)) {
                    game_over = true;
                }
            }
            changed = true;
        }
        if (changed) {
            draw_game(&current, cell_size, score, game_over, paused);
            if (paused) {
                draw_pause_overlay();
            }
        }
        mouse_cursor_draw((uint32_t)mouse_x, (uint32_t)mouse_y);
        __asm__ volatile("pause");
    }
}
