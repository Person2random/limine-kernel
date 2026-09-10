#include "./lapic.h"
#include "./lapic_timer.h"
#include "../io/io.h"
#include "../devices/tsc/tsc.h"
#include "../cpu/msr/msr.h"
#include <stdint.h>
void lapic_timer_init(){
  uint32_t timer_config = 0;
  timer_config |= (1 << 5);      //Assign LAPIC timer to interrupt 32
  timer_config |= (1 << 16);     //Mask (disable) timer by default
  timer_config |= (2 << 17);     //Set timer mode to TSC deadline mode
   
  lapic_write(LAPIC_LVT_TIMER, timer_config); //Write configuration
}

void mask_timer(){
  uint32_t org = lapic_read(LAPIC_LVT_TIMER);
  
  lapic_write(LAPIC_LVT_TIMER, org | (1 << 16));
}

///Although the name is unmask this function also serves the purpose of setting the TSC deadline
///This is because in this kernel the two should be considered part of the same subsystem despite not being so
void unmask_timer(uint64_t deadline){
  uint32_t mask =~(1 << 16);
  uint32_t org = lapic_read(LAPIC_LVT_TIMER);
  lapic_write(LAPIC_LVT_TIMER, org & mask);
  wrmsr(0x6E0, deadline); //0x6E0 is the TSC deadline register
}
