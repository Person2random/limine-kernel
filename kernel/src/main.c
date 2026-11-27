#include "./gdt/gdt.h"
#include "./graphics/graphics.h"
#include "./mm/pmm.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "./terminal/terminal.h"
#include "./idt/idt.h"
#include "./mm/vmm.h"
#include "./pic/pic.h"
#include "./devices/pit/pit.h"
#include "./lapic/lapic.h"
#include "mm/valloc/valloc.h"
#include "./kstd/kstd.h"
#include "./lapic/lapic_timer.h"
#include "./cpu/cpuid.h"
#include "./devices/tsc/tsc.h"
#include "./timer/timer.h"
#include "./cpu/context/context.h"
#include "./keyboard/keyboard.h"
#include "./mouse/mouse.h"
#include "./shell/shell.h"
// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.
// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.
// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
// Halt and catch fire function.


#define STACK_SIZE 4096
static void hcf(void) {
	for (;;) {
#if defined (__x86_64__)
		asm ("sti;hlt");
#elif defined (__aarch64__) || defined (__riscv)
		asm ("wfi");
#elif defined (__loongarch64)
		asm ("idle 0");
#endif
	}
}


extern uint64_t rust_test(void);
// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {

	graphics_init();
	terminal_init(true);
	print("Welcome to tung tung larp OS\n");
	print("Initilazing GDT...\n");
	init_gdt();
	print("Initializng IDT...\n");
	idt_init();
	init_pmm();
	print("PMM init\n");
	valloc_init();
	print("Valloc init\n");
	lapic_init();
	lapic_timer_init();
	print("Lapic init\n");
	init_tsc();
	print("TSC timer init\n");
	if (!ps2_keyboard_init()) {
		kernel_panic("Failed to initalize PS/2 keyboard driver",0x3);
	}
	print("PS/2 keyboard IRQ ready\n");
	if (!ps2_mouse_init()) {
		print("PS/2 mouse unavailable; keyboard controls still work\n");
	} else {
		print("PS/2 mouse IRQ ready\n");
	}
	__asm__ volatile("sti"); //After enabling the LAPIC and timer subsystem, This would be a great time to set interrupts
				 //

	if(rust_test() != 0xDEADBEEF){
		kernel_panic("Interop with other language(s) is unsafe",0x2);
	}

	print("Done\n");
	shell_run();
	hcf();


}
