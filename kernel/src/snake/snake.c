// Made by janctf - 2026 - 27th Of November
#include "snake.h"
#include "../devices/tsc/tsc.h"
#include "../graphics/graphics.h"
#include "../keyboard/keyboard.h"
#include "../terminal/terminal.h"
#include <stdbool.h>
#include <stdint.h>

#define SNAKE_CAPACITY 65536
#define HEADER_HEIGHT 80
#define INITIAL_CELL_SIZE 16
#define MAX_GRID_DIMENSION 256
#define FRAME_DELAY_MS 120

#define COLOR_BACKGROUND 0x101820
#define COLOR_BOARD 0x182830
#define COLOR_SNAKE 0x32CD72
#define COLOR_HEAD 0x7CFC9A
#define COLOR_FOOD 0xFF5C5C
#define COLOR_TEXT 0xFFFFFF
#define COLOR_TEXT_BG 0x000000

enum direction {
    DIRECTION_UP,
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT
};

static uint16_t snake_x[SNAKE_CAPACITY];
static uint16_t snake_y[SNAKE_CAPACITY];

struct game {
    uint32_t cell_size;
    uint32_t columns;
    uint32_t rows;
    uint32_t length;
    uint32_t head;
    uint32_t food_x;
    uint32_t food_y;
    enum direction direction;
    uint64_t random_state;
};

static void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      uint32_t color) {
    for (uint32_t row = 0; row < h; row++) {
        for (uint32_t column = 0; column < w; column++) {
            put_pixel(x + column, y + row, color);
        }
    }
}

static void draw_cell(const struct game *game, uint32_t x, uint32_t y,
                      uint32_t color) {
    fill_rect(x * game->cell_size, HEADER_HEIGHT + y * game->cell_size,
              game->cell_size, game->cell_size, color);
}

static void draw_score(uint32_t score) {
    char digits[10];
    uint32_t count = 0;

    do {
        digits[count++] = '0' + score % 10;
        score /= 10;
    } while (score != 0 && count < sizeof(digits));

    put_char(0, 2, 'S', COLOR_TEXT, COLOR_TEXT_BG);
    put_char(1, 2, 'C', COLOR_TEXT, COLOR_TEXT_BG);
    put_char(2, 2, 'O', COLOR_TEXT, COLOR_TEXT_BG);
    put_char(3, 2, 'R', COLOR_TEXT, COLOR_TEXT_BG);
    put_char(4, 2, 'E', COLOR_TEXT, COLOR_TEXT_BG);
    put_char(5, 2, ':', COLOR_TEXT, COLOR_TEXT_BG);
    for (uint32_t i = 0; i < sizeof(digits); i++) {
        put_char(7 + i, 2, ' ', COLOR_TEXT, COLOR_TEXT_BG);
    }
    for (uint32_t i = 0; i < count; i++) {
        put_char(7 + i, 2, digits[count - i - 1], COLOR_TEXT, COLOR_TEXT_BG);
    }
}

static void draw_game_over(void) {
    put_char(0, 3, 'G', COLOR_FOOD, COLOR_TEXT_BG);
    put_char(1, 3, 'A', COLOR_FOOD, COLOR_TEXT_BG);
    put_char(2, 3, 'M', COLOR_FOOD, COLOR_TEXT_BG);
    put_char(3, 3, 'E', COLOR_FOOD, COLOR_TEXT_BG);
    put_char(5, 3, 'O', COLOR_FOOD, COLOR_TEXT_BG);
    put_char(6, 3, 'V', COLOR_FOOD, COLOR_TEXT_BG);
    put_char(7, 3, 'E', COLOR_FOOD, COLOR_TEXT_BG);
    put_char(8, 3, 'R', COLOR_FOOD, COLOR_TEXT_BG);
}

static uint32_t random_cell(struct game *game, uint32_t limit) {
    // Xorshift is not cryptographic, but the snake has not asked for secrets.
    uint64_t value = game->random_state;
    value ^= value << 13;
    value ^= value >> 7;
    value ^= value << 17;
    game->random_state = value;
    return (uint32_t)(value % limit);
}

static bool cell_is_snake(const struct game *game, uint32_t x, uint32_t y) {
    for (uint32_t i = 0; i < game->length; i++) {
        uint32_t index = (game->head + SNAKE_CAPACITY - i) % SNAKE_CAPACITY;
        if (snake_x[index] == x && snake_y[index] == y) {
            return true;
        }
    }
    return false;
}

static void place_food(struct game *game) {
    uint32_t cells = game->columns * game->rows;
    uint32_t start = random_cell(game, cells);
    for (uint32_t offset = 0; offset < cells; offset++) {
        uint32_t candidate = (start + offset) % cells;
        uint32_t x = candidate % game->columns;
        uint32_t y = candidate / game->columns;
        if (!cell_is_snake(game, x, y)) {
            game->food_x = x;
            game->food_y = y;
            draw_cell(game, x, y, COLOR_FOOD);
            return;
        }
    }
}

static bool read_direction(enum direction *direction, bool *restart) {
    enum direction current = *direction;
    bool changed = false;

    *restart = false;
    struct keyboard_event event;
    while (keyboard_read_event(&event)) {
        if (!event.pressed) {
            continue;
        }
        uint8_t scancode = event.scancode;
        enum direction requested = current;
        bool recognized = true;
        if (event.extended) {
            switch (scancode) {
                case 0x48: requested = DIRECTION_UP; break;
                case 0x4D: requested = DIRECTION_RIGHT; break;
                case 0x50: requested = DIRECTION_DOWN; break;
                case 0x4B: requested = DIRECTION_LEFT; break;
                default: recognized = false; break;
            }
        } else {
            switch (scancode) {
                case 0x11:
                    requested = DIRECTION_UP; break;
                case 0x20:
                    requested = DIRECTION_RIGHT; break;
                case 0x1F:
                    requested = DIRECTION_DOWN; break;
                case 0x1E:
                    requested = DIRECTION_LEFT; break;
                case 0x13:
                    *restart = true;
                    continue;
                default: recognized = false; break;
            }
        }

        if (recognized && !changed && requested != ((current + 2) % 4)) {
            *direction = requested;
            changed = true;
        }
    }
    return changed;
}

static bool wait_for_start(void) {
    print("Press S or ENTER to start\n");
    for (;;) {
        struct keyboard_event event;
        if (!keyboard_read_event(&event)) {
            __asm__ volatile("pause");
            continue;
        }
        if (keyboard_take_app_exit()) {
            return false;
        }
        if (event.pressed && !event.extended &&
            (event.scancode == 0x1F || event.scancode == 0x1C)) {
            return true;
        }
    }
}

static void reset_game(struct game *game) {
    game->length = 3;
    game->head = 2;
    game->direction = DIRECTION_RIGHT;
    snake_x[2] = game->columns / 2;
    snake_y[2] = game->rows / 2;
    snake_x[1] = snake_x[2] - 1;
    snake_y[1] = snake_y[2];
    snake_x[0] = snake_x[2] - 2;
    snake_y[0] = snake_y[2];

    fill_rect(0, HEADER_HEIGHT, (uint32_t)width,
              (uint32_t)height - HEADER_HEIGHT, COLOR_BOARD);
    for (uint32_t x = 0; x < (uint32_t)width / 8; x++) {
        put_char(x, 2, ' ', COLOR_TEXT, COLOR_TEXT_BG);
        put_char(x, 3, ' ', COLOR_TEXT, COLOR_TEXT_BG);
    }
    for (uint32_t i = 0; i < game->length; i++) {
        uint32_t index = (game->head + SNAKE_CAPACITY - i) % SNAKE_CAPACITY;
        draw_cell(game, snake_x[index], snake_y[index],
                  i == 0 ? COLOR_HEAD : COLOR_SNAKE);
    }
    draw_score(0);
    place_food(game);
}

void snake_run(void) {
    if (width > UINT32_MAX || height > UINT32_MAX ||
        width < INITIAL_CELL_SIZE * 4 ||
        height <= HEADER_HEIGHT + INITIAL_CELL_SIZE * 3) {
        print("Framebuffer is too small to play Snake.\n");
        return;
    }

    struct game game = {
        .cell_size = INITIAL_CELL_SIZE,
        .random_state = rdtsc() | 1
    };
    uint32_t screen_width = (uint32_t)width;
    uint32_t screen_height = (uint32_t)height;
    while (screen_width / game.cell_size > MAX_GRID_DIMENSION ||
           (screen_height - HEADER_HEIGHT) / game.cell_size >
               MAX_GRID_DIMENSION) {
        game.cell_size++;
    }
    game.columns = screen_width / game.cell_size;
    game.rows = (screen_height - HEADER_HEIGHT) / game.cell_size;
    if (game.columns < 4 || game.rows < 4) {
        print("Framebuffer is too small to play Snake.\n");
        return;
    }

    fill_rect(0, 0, screen_width, screen_height, COLOR_BACKGROUND);
    terminal_init(false);
    print("SNAKE - Use arrow keys or WASD to move\n");
    print("Eat the red food. Press R to restart after a crash or win.\n");
    if (!wait_for_start()) {
        return;
    }
    // Discard the impatient key presses from the instructions screen.
    struct keyboard_event pending_event;
    while (keyboard_read_event(&pending_event)) {
    }

    reset_game(&game);
    bool game_over = false;
    bool won = false;
    uint64_t frame_ticks = (tsc_get_freq() / 1000) * FRAME_DELAY_MS;
    if (frame_ticks == 0) {
        print("Snake needs a working TSC timer.\n");
        return;
    }
    uint64_t next_frame = rdtsc() + frame_ticks;

    for (;;) {
        bool restart;
        read_direction(&game.direction, &restart);
        if (keyboard_take_app_exit()) {
            return;
        }
        if (restart && (game_over || won)) {
            reset_game(&game);
            game_over = false;
            won = false;
            next_frame = rdtsc() + frame_ticks;
            continue;
        }
        if (rdtsc() < next_frame) {
            __asm__ volatile("pause");
            continue;
        }
        next_frame = rdtsc() + frame_ticks;
        if (game_over || won) {
            continue;
        }

        uint32_t head_x = snake_x[game.head];
        uint32_t head_y = snake_y[game.head];
        uint32_t next_x = head_x;
        uint32_t next_y = head_y;
        switch (game.direction) {
            case DIRECTION_UP:    if (next_y == 0) game_over = true; else next_y--; break;
            case DIRECTION_RIGHT: if (++next_x >= game.columns) game_over = true; break;
            case DIRECTION_DOWN:  if (++next_y >= game.rows) game_over = true; break;
            case DIRECTION_LEFT:  if (next_x == 0) game_over = true; else next_x--; break;
        }
        if (game_over) {
            draw_game_over();
            continue;
        }

        bool eating = next_x == game.food_x && next_y == game.food_y;
        uint32_t segments_to_check = game.length - (eating ? 0 : 1);
        for (uint32_t i = 0; i < segments_to_check; i++) {
            uint32_t index = (game.head + SNAKE_CAPACITY - i) % SNAKE_CAPACITY;
            if (snake_x[index] == next_x && snake_y[index] == next_y) {
                game_over = true;
                break;
            }
        }
        if (game_over) {
            draw_game_over();
            continue;
        }

        uint32_t tail = (game.head + SNAKE_CAPACITY - game.length + 1) %
                        SNAKE_CAPACITY;
        uint32_t next_head = (game.head + 1) % SNAKE_CAPACITY;
        draw_cell(&game, head_x, head_y, COLOR_SNAKE);
        game.head = next_head;
        snake_x[game.head] = next_x;
        snake_y[game.head] = next_y;
        if (eating) {
            game.length++;
        } else {
            draw_cell(&game, snake_x[tail], snake_y[tail], COLOR_BOARD);
        }
        draw_cell(&game, next_x, next_y, COLOR_HEAD);

        if (eating) {
            draw_score(game.length - 3);
            if (game.length == game.columns * game.rows) {
                won = true;
                put_char(0, 3, 'Y', COLOR_HEAD, COLOR_TEXT_BG);
                put_char(1, 3, 'O', COLOR_HEAD, COLOR_TEXT_BG);
                put_char(2, 3, 'U', COLOR_HEAD, COLOR_TEXT_BG);
                put_char(4, 3, 'W', COLOR_HEAD, COLOR_TEXT_BG);
                put_char(5, 3, 'I', COLOR_HEAD, COLOR_TEXT_BG);
                put_char(6, 3, 'N', COLOR_HEAD, COLOR_TEXT_BG);
            } else {
                place_food(&game);
            }
        }
    }
}
