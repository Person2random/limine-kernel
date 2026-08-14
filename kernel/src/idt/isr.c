#include "../pic/pic.h"
#include "../terminal/terminal.h"
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


void pit_handler(void){
  debug_print("Tick"); //Dont actually keep this
 // pic_eoi(0); Replace with IO/APIC EOI
}

__attribute__((noreturn))
void exception_handler(struct interrupt_frame* frame){
  __asm__ volatile("cli;hlt");
  while(1);
}


void interrupt_handler(struct interrupt_frame* frame){
  if(frame->exception == 14){
    print("PF");
  }
  if(frame->exception < 32){
    exception_handler(frame);
  }
  switch (frame->exception) {
    case 32:
      pit_handler();
      break;
  }
  return;
}
