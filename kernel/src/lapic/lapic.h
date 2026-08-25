#include <stdint.h>
#ifndef LAPIC_H
#define LAPIC_H
#define LAPIC_ID              0x020
#define LAPIC_VERSION         0x030
#define LAPIC_TPR             0x080
#define LAPIC_EOI             0x0B0
#define LAPIC_SVR             0x0F0

#define LAPIC_LVT_TIMER       0x320
#define LAPIC_INITIAL_COUNT   0x380
#define LAPIC_CURRENT_COUNT   0x390
#define LAPIC_DIVIDE_CONFIG   0x3E0
void lapic_init();
uint32_t lapic_read(uint32_t reg);
void lapic_write(uint32_t reg, uint32_t value);
;
;
;
;
;
;

#endif
