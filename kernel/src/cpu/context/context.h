#include <stdint.h>
#ifndef CONTEXT_H
#define CONTEXT_H
struct thread_context {
    // General-purpose registers
    uint64_t r15, r14, r13, r12;
    uint64_t r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi;
    uint64_t rdx, rcx, rbx, rax;

    // Interrupt return frame
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};
void context_switch(struct thread_context* context,uint8_t ring);
void test_thread(void);
;
;
;
;
;
;
;
;
;
#endif
