#include <stdint.h>
#ifndef PMM_H
#define PMM_H
void init_pmm(void);
void pmm_free_page(uint64_t address);
uint64_t pmm_alloc_page(void);
#endif
