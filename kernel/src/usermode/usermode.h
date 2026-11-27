#ifndef USERMODE_H
#define USERMODE_H

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
    uint64_t rsp;
    uint64_t ss;
};

void user_mode_run_demo(void);
void user_mode_syscall(struct interrupt_frame *frame);
void user_mode_abort(struct interrupt_frame *frame);

#endif
