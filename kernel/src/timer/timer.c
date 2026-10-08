#include "../lapic/lapic_timer.h"
#include "../devices/tsc/tsc.h"
#include "../lapic/lapic.h"
/// Note that this is a blocking wait
/// Dont use this for a scheduler
/// Hardware interrupts can still happen during the wait
/// Triggers vector 32
void sleep(uint64_t ms) {
  set_lapic_timer_vector(32); //Interrupt handler which does nothing
  uint64_t time_in_ticks = ms * (tsc_get_freq() / 1000);
  uint64_t deadline = time_in_ticks + rdtsc();
  unmask_timer(deadline);
  while(rdtsc() <= deadline){
    __asm__ volatile("hlt");
  }
}


///Sets the timer to wake up after ms
///Non blocking
///DO use this for a scheduler
///Triggers vector 33
///Hardware interrupts still do whatever they want
void timer_schedule(uint64_t ms){
  set_lapic_timer_vector(33);
  uint64_t time_in_ticks = ms * (tsc_get_freq() / 1000);
  uint64_t deadline = time_in_ticks + rdtsc();
  unmask_timer(deadline);
}
