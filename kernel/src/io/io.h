#include <stdint.h>

/*
  Dont touch this, We7yat omak
*/

/**
 * Send a byte to a specific I/O port.
 * @param port The 16-bit I/O port address.
 * @param data The 8-bit data byte to send.
 */
static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

/**
 * Read a byte from a specific I/O port.
 * @param port The 16-bit I/O port address.
 * @return The 8-bit data byte read from the port.
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

