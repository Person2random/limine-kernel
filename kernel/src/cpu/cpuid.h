#include <stdint.h>

#ifndef CPUID_H
#define CPUID_H
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_regs_t;

static inline void cpuid(uint32_t leaf, uint32_t subleaf, cpuid_regs_t *regs) {
    __asm__ volatile (
        "cpuid"
        : "=a"(regs->eax), "=b"(regs->ebx), "=c"(regs->ecx), "=d"(regs->edx)
        : "a"(leaf), "c"(subleaf)
    );
}
#endif
