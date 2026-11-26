#include "../terminal/terminal.h"

void panic(char* message, uint16_t code){
	__asm__ volatile("cli");
	print("!!! KERNEL PANIC !!!");
	__asm__ volatile("hlt");
}
