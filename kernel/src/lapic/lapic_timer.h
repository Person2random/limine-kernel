#include <stdint.h>

#ifndef LAPIC_TIMER_H
#define LAPIC_TIMER_H

#define LAPIC_LVT_TIMER       0x320
#define LAPIC_TIMER_INIT      0x380
#define LAPIC_TIMER_CURRENT   0x390
#define LAPIC_TIMER_DIVIDE    0x3E0


#define LAPIC_TIMER_VECTOR 32


void lapic_timer_init();
void deactivate_timer();
void activate_timer();
;
;
;
;
;
;
;
;
;
;
;
;
;
;
#endif
