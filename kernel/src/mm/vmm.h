#include <stdint.h>
#ifndef VMM_H
#define VMM_H

#define VMM_PRESENT        (1ULL << 0)
#define VMM_WRITE          (1ULL << 1)
#define VMM_USER           (1ULL << 2)
#define VMM_WRITE_THROUGH  (1ULL << 3)
#define VMM_CACHE_DISABLE  (1ULL << 4)
#define VMM_ACCESSED       (1ULL << 5)
#define VMM_DIRTY          (1ULL << 6)
#define VMM_HUGE           (1ULL << 7)
#define VMM_GLOBAL         (1ULL << 8)
#define VMM_NX             (1ULL << 63)


#define VMM_FLAG_MASK 0x800000000000019FULL

void vmm_map(uint64_t virt, uint64_t physical,uint64_t* pml_p);
void vmm_unmap(uint64_t virt,uint64_t* pml_p);
uint64_t vmm_get_phys(uint64_t virt, uint64_t* pml_p);
#endif
