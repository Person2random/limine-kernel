#include <stdbool.h>
#include <stdint.h>
#ifndef TERMINAL_H
#define TERMINAL_H
void terminal_init(bool debug);
void put_char(uint32_t x, uint32_t y, char c ,uint32_t fg, uint32_t bg);
void write_char(char c);
void terminal_write(char c[]);
void print(char c[]);
void debug_print(char c[]);
void debug_terminal_write(char c[]);
void set_bg(uint32_t bg);
void set_fg(uint32_t fg);
extern bool debug_mode;
#endif
