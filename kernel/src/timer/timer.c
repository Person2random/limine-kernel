#include "../lapic/lapic_timer.h"
#include "../devices/tsc/tsc.h"
#include "../lapic/lapic.h"
/// Note that this is a blocking wait
/// Dont use this for a scheduler
/// Hardware interrupts can still happen during the wait
void sleep(uint64_t ms) {
  uint64_t time_in_ticks = ms * (tsc_get_freq() / 1000);
  uint64_t deadline = time_in_ticks + rdtsc();
  unmask_timer(deadline);
  __asm__ volatile("hlt");
}
