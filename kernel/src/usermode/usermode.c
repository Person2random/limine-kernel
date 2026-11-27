#include "usermode.h"
#include "../gdt/gdt.h"
#include "../kstd/kstd.h"
#include "../memory.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../requests/requests.h"
#include "../terminal/terminal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define USER_CODE_BASE 0x00400000ULL
#define USER_STACK_BASE 0x7FFFF000ULL
#define USER_STACK_TOP 0x80000000ULL
#define USER_PAGE_SIZE 4096
#define USER_FRAME_EXIT UINT64_MAX
#define USER_SYSCALL_WRITE 1
#define USER_SYSCALL_EXIT 2

extern uint8_t user_demo_start[];
extern uint8_t user_demo_end[];
extern uint64_t user_saved_rsp;
extern void user_enter(uint64_t entry, uint64_t stack, uint64_t page_table);

static uint64_t *user_page_table;
static uint64_t user_stack_physical;
static uint8_t user_interrupt_stack[16384] __attribute__((aligned(16)));

static bool initialize_user_address_space(void) {
    if (user_page_table != NULL) {
        return true;
    }

    uint64_t code_size = (uint64_t)(user_demo_end - user_demo_start);
    if (code_size == 0 || code_size > USER_PAGE_SIZE ||
        ((uint64_t)user_demo_start & (USER_PAGE_SIZE - 1)) != 0) {
        return false;
    }

    uint64_t root_physical = pmm_alloc_page();
    uint64_t stack_physical = pmm_alloc_page();
    if (root_physical == 0 || stack_physical == 0) {
        if (root_physical != 0) {
            pmm_free_page(root_physical);
        }
        if (stack_physical != 0) {
            pmm_free_page(stack_physical);
        }
        return false;
    }

    uint64_t hhdm_offset = hhdm_request.response->offset;
    uint64_t *new_page_table =
        (uint64_t *)(root_physical + hhdm_offset);
    uint64_t kernel_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(kernel_cr3));
    uint64_t *kernel_page_table =
        (uint64_t *)((kernel_cr3 & ~0xFFFULL) + hhdm_offset);
    memset(new_page_table, 0, USER_PAGE_SIZE);
    for (uint32_t index = 256; index < 512; index++) {
        new_page_table[index] = kernel_page_table[index];
    }

    uint64_t code_physical =
        vmm_get_phys((uint64_t)user_demo_start, 0) & ~0xFFFULL;
    if (code_physical == 0) {
        pmm_free_page(root_physical);
        pmm_free_page(stack_physical);
        return false;
    }
    user_stack_physical = stack_physical;
    memset((void *)(user_stack_physical + hhdm_offset), 0, USER_PAGE_SIZE);
    vmm_map_flags(USER_CODE_BASE, code_physical, new_page_table,
                  VMM_PRESENT | VMM_USER);
    vmm_map_flags(USER_STACK_BASE, user_stack_physical, new_page_table,
                  VMM_PRESENT | VMM_WRITE | VMM_USER);
    user_page_table = new_page_table;
    return true;
}

void user_mode_run_demo(void) {
    if (!initialize_user_address_space()) {
        print("Could not prepare the ring-3 demo address space.\n");
        return;
    }

    memset((void *)(user_stack_physical + hhdm_request.response->offset), 0,
           USER_PAGE_SIZE);
    gdt_set_rsp0((uint64_t)(user_interrupt_stack + sizeof(user_interrupt_stack)));
    uint64_t user_entry =
        USER_CODE_BASE + ((uint64_t)user_demo_start & (USER_PAGE_SIZE - 1));
    user_enter(user_entry, USER_STACK_TOP, (uint64_t)user_page_table -
                                              hhdm_request.response->offset);
    print("Ring-3 demo returned to the kernel.\n");
}

void user_mode_syscall(struct interrupt_frame *frame) {
    if ((frame->cs & 3) != 3) {
        return;
    }
    if (frame->rax == USER_SYSCALL_WRITE) {
        uint64_t pointer = frame->rdi;
        uint64_t length = frame->rsi;
        if (length > 256 || pointer < USER_CODE_BASE ||
            pointer > USER_CODE_BASE + USER_PAGE_SIZE ||
            length > USER_CODE_BASE + USER_PAGE_SIZE - pointer) {
            frame->rax = UINT64_MAX;
            return;
        }

        char buffer[257];
        const char *user_text = (const char *)pointer;
        for (uint64_t index = 0; index < length; index++) {
            buffer[index] = user_text[index];
        }
        buffer[length] = '\0';
        print(buffer);
        frame->rax = length;
    } else if (frame->rax == USER_SYSCALL_EXIT) {
        frame->exception = USER_FRAME_EXIT;
    } else {
        frame->rax = UINT64_MAX;
    }
}

void user_mode_abort(struct interrupt_frame *frame) {
    char buffer[32];
    print("Ring-3 program faulted; returning to the shell. Vector: ");
    int_to_base_string(frame->exception, buffer, 16);
    print(buffer);
    print("\n");
    frame->exception = USER_FRAME_EXIT;
}
