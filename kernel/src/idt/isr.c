#include <stdint.h>
struct interrupt_frame {
    uint64_t r15, r14, r13, r12;
    uint64_t r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi;
    uint64_t rdx, rcx, rbx, rax;

    uint64_t exception;
    uint64_t error_code;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
};
__attribute__((noreturn))
void exception_handler(struct interrupt_frame* frame){
  __asm__ volatile("cli;hlt");
  while(1);
}
