#include <stdint.h>
#include "../../io/io.h"
#define PIT_FREQUENCY 1193182 //dumbass number

void init_pit(uint32_t freq){
  uint16_t divisor = PIT_FREQUENCY/freq; 


  outb(0x43,0x36); // Magic number bs (This just enables mode 3 in the PIT)

  outb(0x40, divisor & 0xFF);
  outb(0x40, (divisor  >> 8) & 0xFF);

}
