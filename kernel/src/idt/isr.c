#include "../kstd/kstd.h"
#include "../pic/pic.h"
#include "../terminal/terminal.h"
#include "../lapic/lapic.h"
#include "../lapic/lapic_timer.h"
#include "../timer/timer.h"
#include "../mouse/mouse.h"
#include "../usermode/usermode.h"
#include <stdint.h>
extern void ps2_keyboard_irq(void);
extern void ps2_mouse_irq(void);

void lapic_timer_handler(void) {
  lapic_eoi();
}

__attribute__((noreturn))
void exception_handler(struct interrupt_frame* frame){
  char buf[32];
  debug_print("Ouu shi we got an exception");
  int_to_base_string(frame->exception, buf,16);
  debug_terminal_write("Exception: ");
  debug_print(buf);
  int_to_base_string(frame->error_code, buf,16);
  debug_terminal_write("Error code: ");
  debug_print(buf);
  int_to_base_string(frame->rip, buf,16);
  debug_terminal_write("RIP: ");
  debug_print(buf);
  int_to_base_string(frame->cs, buf,16);
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
    int_to_base_string(cr2, buf, 16);
    debug_print(buf);

    debug_print("\nERROR: ");
    int_to_base_string(frame->error_code, buf, 16);
    debug_print(buf);

    debug_print("\nRIP: ");
    int_to_base_string(frame->rip, buf, 16);
    debug_print(buf);

    debug_print("\n");

    while (1)
        asm volatile("hlt");
}
void interrupt_handler(struct interrupt_frame* frame){
  if(frame->exception == 14){
    if (frame->cs & 3) {
      user_mode_abort(frame);
      return;
    }
    pf_handler(frame);
  }
  if(frame->exception < 32){
    if (frame->cs & 3) {
      user_mode_abort(frame);
      return;
    }
    exception_handler(frame);
  }
  switch (frame->exception) {
    case 32:
      lapic_timer_handler();
      break;
    case 65:
      ps2_keyboard_irq();
      lapic_eoi();
      break;
    case 76:
      ps2_mouse_irq();
      lapic_eoi();
      break;
    case 128:
      user_mode_syscall(frame);
      break;
  }
  return;
}
