#include "../kstd/kstd.h"
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

//Kosom this handler its unused now
void pit_handler(void){
  debug_print("Tick"); //Dont actually keep this
 // pic_eoi(0); Replace with IO/APIC EOI
}

__attribute__((noreturn))
void exception_handler(struct interrupt_frame* frame){
  char buf[32];
  debug_print("Ouu shi we got an exception");
  utoa(frame->exception, buf);
  debug_terminal_write("Exception: ");
  debug_print(buf);
  utoa(frame->error_code, buf);
  debug_terminal_write("Error code: ");
  debug_print(buf);
  utoa(frame->rip, buf);
  debug_terminal_write("RIP: ");
  debug_print(buf);
  utoa(frame->cs, buf);
  debug_terminal_write("CS: ");
  debug_print(buf);
  __asm__ volatile("cli;hlt");
  while(1);
}
void pf_handler(struct interrupt_frame* frame) {
    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    char buf[32];

    debug_print("PAGE FAULT\n");

    debug_print("CR2: ");
    utoa(cr2, buf);
    debug_print(buf);

    debug_print("\nERROR: ");
    utoa(frame->error_code, buf);
    debug_print(buf);

    debug_print("\nRIP: ");
    utoa(frame->rip, buf);
    debug_print(buf);

    debug_print("\n");

    while (1)
        asm volatile("hlt");
}
void interrupt_handler(struct interrupt_frame* frame){
  if(frame->exception == 14){
    pf_handler(frame);
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
