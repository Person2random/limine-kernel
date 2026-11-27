#ifndef GDT_H
#define GDT_H
#include <stdint.h>

void init_gdt(void);
void gdt_set_rsp0(uint64_t rsp0);
#endif
