#include <stdint.h>
#include "../mm/vmm.h"
#include "../mm/valloc/valloc.h"
#include "../pic/pic.h"
#include "../terminal/terminal.h"
#include "../kstd/kstd.h"
#include "../cpu/msr/msr.h"
#include "./lapic.h"
extern uint64_t get_lapic_base(void);
extern uint64_t get_lapic_msr(void);
uint64_t lapic_virt;
void lapic_init(){
  wrmsr(0x1B, get_lapic_msr() | (1ULL << 11));
  // lapic_virt = 0xFFFF800000000000; Keep this here for when youre testing,
  // Total hours spent on lapic is probably in the hundreds now
  lapic_virt = (uint64_t)valloc(1);
  char buf[32];
  if(lapic_virt == 0){
    return;
  }
  uint64_t lapic_phys = get_lapic_base();
  vmm_map(lapic_virt, lapic_phys, 0);
  vmm_set_flags((uint64_t)lapic_virt,
                VMM_PRESENT | VMM_WRITE | VMM_WRITE_THROUGH | VMM_CACHE_DISABLE,
                0);
  lapic_write(LAPIC_TPR, 0);
  lapic_write(LAPIC_SVR, LAPIC_SPURIOUS_VECTOR | (1 << 8));
  lapic_write(LAPIC_LVT_LINT0, 0x700);
  pic_disable();
}


uint32_t lapic_read(uint32_t offset)
{
    volatile uint32_t* reg_addr = (uint32_t*)(lapic_virt+offset);
    return *reg_addr;
}

void lapic_write(uint32_t offset, uint32_t value)
{
    *(volatile uint32_t *)(lapic_virt + offset) = value;
}

void lapic_eoi(void){
  lapic_write(LAPIC_EOI,0);
}
