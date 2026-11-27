#include "shell.h"
#include "../graphics/graphics.h"
#include "../keyboard/keyboard.h"
#include "../snake/snake.h"
#include "../pong/pong.h"
#include "../tetris/tetris.h"
#include "../demo3d/demo3d.h"
#include "../usermode/usermode.h"
#include "../cpu/cpuid.h"
#include "../mm/pmm.h"
#include "../terminal/font.h"
#include "../terminal/terminal.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_COLUMNS 160
#define MAX_ROWS 64
#define MAX_INPUT 128
#define COLOR_BACKGROUND 0x101820
#define COLOR_TEXT 0xFFFFFF
#define COLOR_ACCENT 0x7CFC9A

static char history[MAX_ROWS][MAX_COLUMNS];
static uint32_t history_count;
static uint32_t history_column;
static uint32_t columns;
static uint32_t rows;
static char input[MAX_INPUT];
static uint32_t input_length;

static void clear_framebuffer(void) {
    for (uint32_t y = 0; y < (uint32_t)height; y++) {
        for (uint32_t x = 0; x < (uint32_t)width; x++) {
            put_pixel(x, y, COLOR_BACKGROUND);
        }
    }
}

static void scroll_history(void) {
    for (uint32_t row = 1; row < MAX_ROWS; row++) {
        for (uint32_t column = 0; column < MAX_COLUMNS; column++) {
            history[row - 1][column] = history[row][column];
        }
    }
    for (uint32_t column = 0; column < MAX_COLUMNS; column++) {
        history[MAX_ROWS - 1][column] = '\0';
    }
    if (history_count != 0) {
        history_count--;
    }
}

static void write_output(const char *text) {
    while (*text) {
        char character = *text++;
        if (character == '\r') {
            continue;
        }
        if (character == '\n') {
            history_column = 0;
            if (history_count + 1 >= MAX_ROWS) {
                scroll_history();
            }
            history_count++;
            continue;
        }
        if (history_count >= MAX_ROWS) {
            scroll_history();
        }
        history[history_count][history_column++] = character;
        if (history_column >= columns) {
            history_column = 0;
            if (history_count + 1 >= MAX_ROWS) {
                scroll_history();
            }
            history_count++;
        } else {
            history[history_count][history_column] = '\0';
        }
    }
}

static void render_shell(void) {
    clear_framebuffer();
    uint32_t visible_history = rows > 1 ? rows - 1 : 0;
    uint32_t first = history_count > visible_history
                         ? history_count - visible_history
                         : 0;
    uint32_t y = 0;
    for (uint32_t line = first; line < history_count && y < rows; line++, y++) {
        for (uint32_t x = 0; x < columns && history[line][x]; x++) {
            put_char(x, y, history[line][x], COLOR_TEXT, COLOR_BACKGROUND);
        }
    }
    if (y >= rows) {
        y = rows - 1;
    }
    put_char(0, y, '>', COLOR_ACCENT, COLOR_BACKGROUND);
    put_char(1, y, ' ', COLOR_TEXT, COLOR_BACKGROUND);
    for (uint32_t x = 0; x < input_length && x + 2 < columns; x++) {
        put_char(x + 2, y, input[x], COLOR_TEXT, COLOR_BACKGROUND);
    }
    if (input_length + 2 < columns) {
        put_char(input_length + 2, y, '_', COLOR_ACCENT, COLOR_BACKGROUND);
    }
}

static uint32_t prompt_row(void) {
    return history_count < rows - 1 ? history_count : rows - 1;
}

static void render_prompt_input(void) {
    uint32_t y = prompt_row();
    put_char(0, y, '>', COLOR_ACCENT, COLOR_BACKGROUND);
    put_char(1, y, ' ', COLOR_TEXT, COLOR_BACKGROUND);
    for (uint32_t x = 0; x < input_length && x + 2 < columns; x++) {
        put_char(x + 2, y, input[x], COLOR_TEXT, COLOR_BACKGROUND);
    }
    if (input_length + 2 < columns) {
        put_char(input_length + 2, y, '_', COLOR_ACCENT, COLOR_BACKGROUND);
    }
}

static bool string_equals(const char *left, const char *right) {
    while (*left && *left == *right) {
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

static bool parse_run_app(const char *command, const char *app) {
    const char prefix[] = "run ";
    for (uint32_t i = 0; i < sizeof(prefix) - 1; i++) {
        if (command[i] != prefix[i]) {
            return false;
        }
    }
    return string_equals(command + sizeof(prefix) - 1, app);
}

static void keyboard_test_run(void) {
    clear_framebuffer();
    terminal_init(false);
    print("KEYBOARD TEST - press keys to see their events\n");
    print("Ctrl+C returns to the terminal\n");
    print("Last key: (waiting)\n");

    struct keyboard_event event;
    for (;;) {
        if (keyboard_read_event(&event)) {
            if (keyboard_take_app_exit()) {
                return;
            }
            if (event.pressed) {
                for (uint32_t x = 10; x < columns; x++) {
                    put_char(x, 2, ' ', COLOR_TEXT, COLOR_BACKGROUND);
                }
                if (event.character >= 32 && event.character <= 126) {
                    put_char(10, 2, event.character, COLOR_ACCENT,
                             COLOR_BACKGROUND);
                } else {
                    static const char hex[] = "0123456789ABCDEF";
                    put_char(10, 2, hex[event.scancode >> 4], COLOR_ACCENT,
                             COLOR_BACKGROUND);
                    put_char(11, 2, hex[event.scancode & 0x0F], COLOR_ACCENT,
                             COLOR_BACKGROUND);
                }
                put_char(14, 2, event.extended ? 'E' : ' ', COLOR_TEXT,
                         COLOR_BACKGROUND);
                put_char(16, 2, 'D', COLOR_TEXT, COLOR_BACKGROUND);
                put_char(17, 2, 'N', COLOR_TEXT, COLOR_BACKGROUND);
            } else {
                put_char(16, 2, 'U', COLOR_TEXT, COLOR_BACKGROUND);
                put_char(17, 2, 'P', COLOR_TEXT, COLOR_BACKGROUND);
            }
        } else {
            __asm__ volatile("pause");
        }
    }
}

static void editor_run(void) {
    clear_framebuffer();
    terminal_init(false);
    print("TEXT EDITOR - temporary buffer, nothing is saved\n");
    print("Type text. Ctrl+C returns to the terminal.\n\n");

    uint32_t editor_columns = columns;
    uint32_t editor_rows = rows > 3 ? rows - 3 : 1;
    if (editor_columns == 0 || editor_rows == 0) {
        return;
    }
    uint32_t x = 0;
    uint32_t y = 0;
    // A scratchpad in RAM: the disk driver is welcome to enjoy its day off.
    static char editor_buffer[MAX_ROWS][MAX_COLUMNS];
    for (uint32_t row = 0; row < MAX_ROWS; row++) {
        for (uint32_t column = 0; column < MAX_COLUMNS; column++) {
            editor_buffer[row][column] = '\0';
        }
    }
    put_char(x, y + 3, '_', COLOR_ACCENT, COLOR_BACKGROUND);

    for (;;) {
        struct keyboard_event event;
        if (!keyboard_read_event(&event)) {
            __asm__ volatile("pause");
            continue;
        }
        if (keyboard_take_app_exit()) {
            return;
        }
        if (!event.pressed) {
            continue;
        }
        put_char(x, y + 3, editor_buffer[y][x] ? editor_buffer[y][x] : ' ',
                 COLOR_TEXT, COLOR_BACKGROUND);
        if (event.character == '\b') {
            if (x != 0) {
                x--;
            } else if (y != 0) {
                y--;
                x = editor_columns - 1;
            }
            editor_buffer[y][x] = '\0';
            put_char(x, y + 3, '_', COLOR_ACCENT, COLOR_BACKGROUND);
            continue;
        }
        if (event.character == '\n') {
            x = 0;
            y++;
            if (y >= editor_rows) {
                for (uint32_t row = 1; row < editor_rows; row++) {
                    for (uint32_t column = 0; column < editor_columns; column++) {
                        editor_buffer[row - 1][column] = editor_buffer[row][column];
                    }
                }
                for (uint32_t column = 0; column < editor_columns; column++) {
                    editor_buffer[editor_rows - 1][column] = '\0';
                }
                clear_framebuffer();
                terminal_init(false);
                print("TEXT EDITOR - temporary buffer, nothing is saved\n");
                print("Type text. Ctrl+C returns to the terminal.\n\n");
                for (uint32_t row = 0; row < editor_rows; row++) {
                    for (uint32_t column = 0;
                         column < editor_columns && editor_buffer[row][column];
                         column++) {
                        put_char(column, row + 3, editor_buffer[row][column],
                                 COLOR_TEXT, COLOR_BACKGROUND);
                    }
                }
                y = editor_rows - 1;
            }
            put_char(x, y + 3, '_', COLOR_ACCENT, COLOR_BACKGROUND);
            continue;
        }
        if (event.character < 32 || event.character > 126) {
            put_char(x, y + 3, '_', COLOR_ACCENT, COLOR_BACKGROUND);
            continue;
        }
        editor_buffer[y][x] = event.character;
        put_char(x, y + 3, event.character, COLOR_TEXT, COLOR_BACKGROUND);
        x++;
        if (x >= editor_columns) {
            x = 0;
            y++;
            if (y >= editor_rows) {
                for (uint32_t row = 1; row < editor_rows; row++) {
                    for (uint32_t column = 0; column < editor_columns; column++) {
                        editor_buffer[row - 1][column] = editor_buffer[row][column];
                    }
                }
                for (uint32_t column = 0; column < editor_columns; column++) {
                    editor_buffer[editor_rows - 1][column] = '\0';
                }
                clear_framebuffer();
                terminal_init(false);
                print("TEXT EDITOR - temporary buffer, nothing is saved\n");
                print("Type text. Ctrl+C returns to the terminal.\n\n");
                for (uint32_t row = 0; row < editor_rows; row++) {
                    for (uint32_t column = 0;
                         column < editor_columns && editor_buffer[row][column];
                         column++) {
                        put_char(column, row + 3, editor_buffer[row][column],
                                 COLOR_TEXT, COLOR_BACKGROUND);
                    }
                }
                y = editor_rows - 1;
            }
        }
        put_char(x, y + 3, '_', COLOR_ACCENT, COLOR_BACKGROUND);
    }
}

static void draw_neofetch_small_text(uint32_t x, uint32_t y,
                                     const char *text, uint32_t color) {
    const uint32_t glyph_width = width < 900 ? 3 : 6;
    const uint32_t glyph_height = width < 900 ? 4 : 8;
    const unsigned char *glyphs = font + 4;

    for (uint32_t character = 0; text[character]; character++) {
        const unsigned char *glyph =
            glyphs + (uint8_t)text[character] * 16;
        for (uint32_t row = 0; row < glyph_height; row++) {
            uint32_t pixel_y = y + row;
            if (pixel_y >= height) {
                continue;
            }
            uint8_t bits = glyph[(row * 16) / glyph_height];
            for (uint32_t column = 0; column < glyph_width; column++) {
                uint32_t pixel_x = x + character * glyph_width + column;
                uint32_t source_x = (column * 8) / glyph_width;
                if (pixel_x < width && (bits & (0x80 >> source_x))) {
                    put_pixel(pixel_x, pixel_y, color);
                }
            }
        }
    }
}

static void draw_neofetch_detail(uint32_t column, uint32_t row,
                                 const char *label, const char *value) {
    uint32_t x = column;
    while (*label && x < columns) {
        put_char(x++, row, *label++, COLOR_ACCENT, COLOR_BACKGROUND);
    }
    if (x < columns) {
        put_char(x++, row, ':', COLOR_ACCENT, COLOR_BACKGROUND);
    }
    if (x < columns) {
        put_char(x++, row, ' ', COLOR_TEXT, COLOR_BACKGROUND);
    }
    while (*value && x < columns) {
        put_char(x++, row, *value++, COLOR_TEXT, COLOR_BACKGROUND);
    }
}

static void draw_neofetch_art_line(uint32_t line, const char *text) {
    uint32_t glyph_height = width < 900 ? 4 : 8;
    draw_neofetch_small_text(8, 8 + line * glyph_height, text, COLOR_ACCENT);
}

// Asser forced me to add this, I didn't consent to it, but I guess it makes sense to have it here.
static void format_u64(uint64_t value, char *buffer) {
    char reversed[20];
    uint32_t length = 0;
    do {
        reversed[length++] = '0' + value % 10;
        value /= 10;
    } while (value != 0 && length < sizeof(reversed));
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = reversed[length - i - 1];
    }
    buffer[length] = '\0';
}

static void append_text(char *destination, const char *text) {
    while (*destination) {
        destination++;
    }
    while (*text) {
        *destination++ = *text++;
    }
    *destination = '\0';
}

static void read_cpu_brand(char *brand) {
    cpuid_regs_t regs;
    cpuid(0x80000000, 0, &regs);
    if (regs.eax < 0x80000004) {
        append_text(brand, "Unknown CPU");
        return;
    }

    uint32_t index = 0;
    for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; leaf++) {
        cpuid(leaf, 0, &regs);
        const uint32_t words[] = {regs.eax, regs.ebx, regs.ecx, regs.edx};
        for (uint32_t word = 0; word < 4; word++) {
            for (uint32_t byte = 0; byte < 4; byte++) {
                brand[index++] = (char)(words[word] >> (byte * 8));
            }
        }
    }
    brand[index] = '\0';

    uint32_t first = 0;
    while (brand[first] == ' ') {
        first++;
    }
    if (first != 0) {
        uint32_t offset = 0;
        while (brand[first + offset]) {
            brand[offset] = brand[first + offset];
            offset++;
        }
        brand[offset] = '\0';
    }
    while (index != 0 && brand[index - 1] == ' ') {
        brand[--index] = '\0';
    }
}
// We have our reasons to add this.
static void fastfetch_run(void) {
    static const char *const logo[] = {
        "                                      ---------------",
        "                                 -++==-----------------==-",
        "                                ****+-----------------=+***+",
        "                                **+=---------:..:------=+**@",
        "                                +==-------*@@@@@@@@@@*==+**@",
        "                             -@@@@@@@*++*@@@@@@@@@@@@@%****@",
        "                            @@@@@@@@@@@**@@@@@@*++*%#@@****@",
        "                           *@@@*-*@@@@@=*@@@@=@@@@@  @@@***@",
        "                           -@@@@@@- @@@ -*@@*@@@@@@@.+@@@**%",
        "                            @@@@@@*.*@* -+**%@@@@@@@%@*:-=*@",
        "                            @@@@@@@@@#=:-++==*@@@@@*-  :--*@.",
        "                           :+*@@@*++*%-.-+**+===-----==***%@:",
        "                          .%*=--=**@#*------*@@@@@@@@@@@@@@@-",
        "                           *@@@@@@@@@@@@@@@@@**@@@@@@@@@@@@@-",
        "                            +@@@@@@*@@@@@@@@=  .+@@@@@@@@@@@=",
        "                              #*@@@%@@@@@@@@@@@@@%*%@@@@@@@@+",
        "                               :*@@@@@@@@@***%@@*+**#@@@@@@@+",
        "                                -@@@@@@@@@@@@@*++***%@@@@@@@+",
        "                                 :@@@@@@@@**+++***%@@#%@@@@@*",
        "                                  @@@@@@@@%%#@@@@@@%***#@@@@*",
        "                                 .%@@@@@@@@@@@@@#*******#%@@*",
        "                                 -+@@@@@@@@@@#***********--*+",
        "                                 #=@@@@@@@#************@@***=",
        "                                #@*@@@@@%************%%@@@**-",
        "                                @@@#@@@@*************%%@@@**=",
        "                                @@@+@@@@#************#%@@@@**",
        "                                @@@+@@@@@%#***********%@@@@%*+",
        "                               :@@@+@@@@@@%##*********#@@@%-+=",
        "                               %@@@@@@@@@%%#**********#@@@*=+",
        "                               @@@@@@@@@@%%##*********#@@%=+:",
        "                               *@@@@@@@@@%%###*******#%@@*+*",
        "                               @@+ @@@@@@@%%%###**###%@@*-*.",
        "                               =@+ @@@@@@@%%%#######%#****#",
        "                              .@%@@@@@@@@@@%%%%#####*@@@*%@",
        "                               @@%@@@@@@@@@@%%###%@@@@@*@@@",
        "                               .*#@@@@@@@@@@@%%%%@@@@@@@@",
        "                              :+*@+ @@@@@@@*..=%@@@@@@@=",
        "                             =+*@:  %@@@@@@     @@@%@@@",
        "                           .+*#%    +@@@@@@     #****@%",
        "                          ++*%*     :@#@@@@     *****@*",
        "                        =+**@+      :@*#@@@     *****@*",
        "                       +++*@:       =@**@@@     -****@+",
        "                     =++*%@          @@@@@%      %@@@@*",
        "                    +++*@*           @@@@@@      *@@@@@",
        "                  +++*#@=            #@@@@@      :@@@%@-",
        "                =****@@.              @@@@@       @@%@@*",
        "              -**+*#@@                #@@@@:      #@@@@*",
        "            -*****@@#                 :@@@@*      *@@@@=",
        "          .******@@*                   @@@@*      +@@@@",
        "         ******%@@+                    @@@@#      .@@@@",
        "       *******%@@.                     @%@@@       @@@@",
        "     :******#@@@                      -**%@@%     +%%%@@",
        "    *#****#%@@*                      -+**%@@@@    *+**@*",
        "  =#*****%@@@=                    :-=****%@@@@@@ -=+***@=",
        " #@%**#%@@@@          .**+=-:::--=+****@@@@@@@@*-==+*****",
        "#@@@@@@@@@%          ***+***+=+*****@@@@@@#***===++++++**+",
        "   +@@@@@           :@@#******#@@@@@:       -+********+***+",
        "                        *%@%%*.           :=**#***@**@*#@*@:",
        "                                          #@@@**@@**#*-+: ",
    };
    const uint32_t details_column = width < 1400 ? width / 16 : 82;
    char screen_size[32] = {0};
    char number[24];
    char memory[32] = {0};
    char cpu_brand[49] = {0};

    clear_framebuffer();
    terminal_init(false);
    for (uint32_t row = 0; row < sizeof(logo) / sizeof(logo[0]); row++) {
        draw_neofetch_art_line(row, logo[row]);
    }
// I feel like theres a better way to do this
    format_u64(width, screen_size);
    append_text(screen_size, "x");
    format_u64(height, number);
    append_text(screen_size, number);

    draw_neofetch_detail(details_column, 2, "OS", "Tung Tung Larp OS");
    draw_neofetch_detail(details_column, 3, "Kernel", "ttlOS-0.1");
    draw_neofetch_detail(details_column, 4, "Screen", screen_size);

    format_u64(pmm_get_usable_memory_bytes() / (1024 * 1024), memory);
    append_text(memory, " MiB usable");
    draw_neofetch_detail(details_column, 5, "Memory", memory);

    read_cpu_brand(cpu_brand);
    draw_neofetch_detail(details_column, 6, "CPU", cpu_brand);

    for (;;) {
        struct keyboard_event event;
        if (!keyboard_read_event(&event)) {
            __asm__ volatile("pause");
            continue;
        }
        if (keyboard_take_app_exit()) {
            return;
        }
    }
}

static void clear_history(void) {
    for (uint32_t row = 0; row < MAX_ROWS; row++) {
        for (uint32_t column = 0; column < MAX_COLUMNS; column++) {
            history[row][column] = '\0';
        }
    }
    history_count = 0;
    history_column = 0;
    input_length = 0;
}

static void execute_command(void) {
    input[input_length] = '\0';
    write_output("> ");
    write_output(input);
    write_output("\n");

    if (string_equals(input, "help")) {
        write_output("Commands: help, cmds, apps, run <app>, clear\n");
        write_output("Apps: snake, pong, tetris, 3d, keyboard, editor,\n");
        write_output("      fastfetch, userspace\n");
        write_output("Pong: W/S and arrow keys. P or click Pause/Resume.\n");
        write_output("Tetris: arrows, Z/X rotate, Space drops, R restarts.\n");
        write_output("       P or click Pause/Resume. Ctrl+C exits any app.\n");
    } else if (string_equals(input, "cmds")) {
        write_output("cmds - list commands\n");
        write_output("help - commands, apps, and controls\n");
        write_output("apps - list apps\n");
        write_output("run <app> - run an app\n");
        write_output("clear - clear the terminal\n");
    } else if (string_equals(input, "apps")) {
        write_output("snake - Snake game\n");
        write_output("pong - two-player Pong\n");
        write_output("tetris - falling block game\n");
        write_output("3d - rotating wireframe cube\n");
        write_output("userspace - run a ring-3 syscall demo\n");
        write_output("keyboard - keyboard input test\n");
        write_output("editor - temporary text editor\n");
        write_output("fastfetch - show system information\n");
    } else if (parse_run_app(input, "snake")) {
        render_shell();
        snake_run();
        write_output("Snake closed.\n");
    } else if (parse_run_app(input, "pong")) {
        render_shell();
        pong_run();
        write_output("Pong closed.\n");
    } else if (parse_run_app(input, "tetris")) {
        render_shell();
        tetris_run();
        write_output("Tetris closed.\n");
    } else if (parse_run_app(input, "3d")) {
        render_shell();
        demo3d_run();
        write_output("3D demo closed. The cube has returned to flatland.\n");
    } else if (parse_run_app(input, "userspace")) {
        render_shell();
        user_mode_run_demo();
        write_output("Ring-3 demo returned via write and exit syscalls.\n");
    } else if (parse_run_app(input, "keyboard")) {
        render_shell();
        keyboard_test_run();
        write_output("Keyboard test closed.\n");
    } else if (parse_run_app(input, "editor")) {
        render_shell();
        editor_run();
        write_output("Editor closed. Nothing was saved.\n");
    } else if (parse_run_app(input, "fastfetch") ||
               parse_run_app(input, "neofetch")) {
        render_shell();
        fastfetch_run();
        write_output("Fastfetch closed.\n");
    } else if (string_equals(input, "clear")) {
        clear_history();
    } else if (input_length != 0) {
        write_output("Unknown command. Type cmds to list commands.\n");
    }
    input_length = 0;
}

void shell_run(void) {
    uint32_t framebuffer_columns = (uint32_t)(width / 8);
    uint32_t framebuffer_rows = (uint32_t)(height / 16);
    columns = framebuffer_columns < MAX_COLUMNS ? framebuffer_columns : MAX_COLUMNS;
    rows = framebuffer_rows < MAX_ROWS ? framebuffer_rows : MAX_ROWS;
    if (columns > MAX_INPUT + 2) {
        columns = MAX_INPUT + 2;
    }
    if (columns < 3 || rows < 2) {
        return;
    }

    clear_history();
    write_output("tung tung larp OS terminal\n");
    write_output("Type cmds for commands or apps for applications.\n");
    render_shell();

    for (;;) {
        struct keyboard_event event;
        if (!keyboard_read_event(&event)) {
            __asm__ volatile("pause");
            continue;
        }
        keyboard_take_app_exit();
        if (!event.pressed || event.extended) {
            continue;
        }
        if (event.character == '\n') {
            execute_command();
            render_shell();
        } else if (event.character == '\b') {
            if (input_length != 0) {
                input_length--;
                input[input_length] = '\0';
                render_prompt_input();
            }
        } else if (event.character >= 32 && event.character <= 126 &&
                   input_length + 1 < MAX_INPUT &&
                   input_length + 3 < columns) {
            input[input_length++] = event.character;
            input[input_length] = '\0';
            render_prompt_input();
        }
    }
}
