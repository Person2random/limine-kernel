#include "../terminal/terminal.h"
#include "../kstd/kstd.h"
void kernel_panic(const char* message, uint16_t code){
	__asm__ volatile("cli");	
	set_cursor(0,0);
	for(int x = 0; x < max_x; x++){
		for(int y = 0; y < max_y; y++){
			print(" ");
		}
	}
	set_cursor(0,0);
	print("!!! KERNEL PANIC !!!\n");
	print(message);
	print("\n");
	char buf[32];
	int_to_base_string(code,buf,16);
	print("Error code: ");
	print(buf);

	__asm__ volatile("hlt");

}
