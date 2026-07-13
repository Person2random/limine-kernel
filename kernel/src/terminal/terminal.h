#include <stdint.h>
#ifndef TERMINAL_H
#define TERMINAL_H
void terminal_init(void);
void put_char(uint32_t x, uint32_t y, char c ,uint32_t fg, uint32_t bg);
void write_char(char c);
void terminal_write(char c[]);
void print(char c[]);

#endif
