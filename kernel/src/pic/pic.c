#include "pic.h"
#include <stdint.h>
#include "../io/io.h"
void disable_init(){
  outb(PIC1_DATA, 0xFF); // Mask everything
  outb(PIC2_DATA,0xFF);
  }
