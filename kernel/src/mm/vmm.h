#include <stdint.h>
#ifndef VMM_H
#define VMM_H

void vmm_map(uint64_t virt, uint64_t physical);
void vmm_unmap(uint64_t virt);
#endif
