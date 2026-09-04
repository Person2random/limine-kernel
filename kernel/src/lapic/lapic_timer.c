#include "./lapic.h"
#include "./lapic_timer.h"
#include "../io/io.h"
#include "../devices/tsc/tsc.h"
#include <stdint.h>
void lapic_timer_init(uint32_t initial_count){
  lapic_write(LAPIC_LVT_TIMER, LAPIC_TIMER_VECTOR | (1 << 17));
  lapic_write(LAPIC_TIMER_DIVIDE, 0x3);
  lapic_write(LAPIC_TIMER_INIT, initial_count);
}




