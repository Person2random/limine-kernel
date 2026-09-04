#include "./lapic.h"
#include "./lapic_timer.h"
#include "../io/io.h"
#include "../devices/tsc/tsc.h"
#include <stdint.h>
void lapic_timer_init(){
  lapic_write(LAPIC_LVT_TIMER, LAPIC_TIMER_VECTOR); //Enable TSC deadline mode
}

void activate_timer(){
  lapic_write(LAPIC_LVT_TIMER, LAPIC_TIMER_VECTOR | (1 << 16));
}


void deactivate_timer(){
  lapic_write(LAPIC_LVT_TIMER, LAPIC_TIMER_VECTOR | (0 << 16));
}



