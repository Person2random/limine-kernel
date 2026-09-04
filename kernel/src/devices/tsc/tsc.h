#include <stdint.h>

#ifndef TSC_H
#define TSC_H
uint64_t rdtsc(void);
void init_tsc();
__attribute__((noinline))
uint64_t tsc_get_freq();
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
