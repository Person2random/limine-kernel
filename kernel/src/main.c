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
static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
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
    print("GDT init");
    print("Doing coding exercise");
    int height = 5;
    int start_col = 5;
    int starcount = 0;
    for(int i = 5; i > 0; i--){
        for(int j = 0; j <= start_col; j++){
            write_char(' ');
        }
        for(int j = 0; j <= starcount; j++){
            write_char('*');
        }
        starcount += 2;
        start_col--;
        print("");
    }
    print("\n^^^ This should be a triangle");
    print("Initializng IDT...");
    idt_init();
    print("IDT init");
    print("Initializing PMM...");
    init_pmm();
    print("PMM init");

    print("Starting PMM test");
    uint64_t a = pmm_alloc_page();
    uint64_t b = pmm_alloc_page();
    pmm_free_page(a);
    uint64_t c = pmm_alloc_page();
    if(c != a){
        print("Something is wrong with paging. It isnt safe to continue");
        __asm__ volatile("sti;hlt");
    }
    pmm_free_page(b);
    pmm_free_page(c);
    print("PMM test successful");
    // We're done, just hang...
    uint64_t phys = pmm_alloc_page();

    uint64_t virt = 0x40000000;
    vmm_map(virt, phys);
    uint64_t *ptr = (uint64_t *)virt;
    *ptr = 0xDEADBEEFCAFEBABE;
    if (*ptr == 0xDEADBEEFCAFEBABE) {
      debug_terminal_write("VMM MAP WORKS\n");
    } else {
        debug_terminal_write("VMM MAP FAILED\n");
    }
    hcf();
}
