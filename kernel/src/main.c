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

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
   /*
    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    // Print a nice pattern to screen as an example.
    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    volatile uint32_t *fb_ptr = framebuffer->address;
    for (size_t y = 0; y < framebuffer->height; y++) {
        for (size_t x = 0; x < framebuffer->width; x++) {
            uint32_t nX = x * 255 / framebuffer->width;
            uint32_t nY = y * 255 / framebuffer->height;
            fb_ptr[y * (framebuffer->pitch / 4) + x] = (nY << 8) | nX;
        }
    }
*/
    graphics_init();
    terminal_init(true);
    print("Welcome to tung tung larp OS");
    print("Initilazing GDT...");
    init_gdt();
    print("Initializng IDT...");
    idt_init();
    init_pmm();
    print("PMM init");
    valloc_init();
    print("Valloc init");
    lapic_init();
    lapic_timer_init();
    print("Lapic init");
    init_tsc();
    print("TSC timer init");
    __asm__ volatile("sti"); //After enabling the LAPIC and timer subsystem, This would be a great time to set interrupts
    sleep(50);
    print("Done");
    print("About to switch context");
    uint8_t test_stack[STACK_SIZE] __attribute__((aligned(16)));

    struct thread_context test_context = {
        .r15 = 0,
        .r14 = 0,
        .r13 = 0,
        .r12 = 0,
        .r11 = 0,
        .r10 = 0,
        .r9 = 0,
        .r8 = 0,

        .rbp = 0,
        .rdi = 0,
        .rsi = 0,
        .rdx = 0,
        .rcx = 0,
        .rbx = 0,
        .rax = 0,

        .rip = (uint64_t)test_thread,
        .cs = 0x08,
        .rflags = 0x202,
        .rsp = (uint64_t)(test_stack + STACK_SIZE-8),
        .ss = 0x10};
    char buf[32];
    int_to_base_string(test_context.rsp, buf, 16);
    print(buf);
    context_switch(&test_context, 0);
    
    hcf();
}
