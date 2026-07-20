#include "./idt.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
  uint16_t limit;
  uint64_t base;
}__attribute__((packed)) idt_r;

typedef struct {
  uint16_t isr_low;
  uint16_t segment;
  uint8_t ist;
  uint8_t options;
  uint16_t isr_mid;
  uint32_t isr_high;
  uint32_t reserved; // Set to 0 always
} __attribute__((packed)) idt_entry;

idt_entry idt[256];
idt_r idt_pointer;

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags){
  idt_entry* descriptor = &idt[vector];
  descriptor->isr_low =(uint64_t)isr & 0xFFFF;
  descriptor -> segment = 0x08;
  descriptor -> ist = 0;
  descriptor -> options = flags;
  descriptor -> isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
  descriptor -> isr_high = ((uint64_t) isr >> 32) & 0xFFFFFFFF;
  descriptor -> reserved = 0;
}

extern void* isr_stub_table[];
static bool vectors[256];

void idt_init(void){
  idt_pointer.base = (uintptr_t)&idt[0];
  idt_pointer.limit = (uint16_t)sizeof(idt_entry) * 256 - 1;
  for(uint8_t vector = 0; vector < 32; vector++){
    idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
    vectors[vector] = true;
  }
  __asm__ volatile ("lidt %0": : "m"((idt_pointer)));
 // __asm__ volatile ("sti");
    
}
