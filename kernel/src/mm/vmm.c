#include <stdint.h>
#include "../requests/requests.h"
#include "./pmm.h"
void vmm_map(uint64_t virt, uint64_t physical){
  uint64_t hhdm_offset = hhdm_request.response->offset;
  uint64_t pml4_index = (virt >> 39) & 0x1FF;
  uint64_t pdpt_index = (virt >> 30) & 0x1FF;
  uint64_t pd_index = (virt >> 21) & 0x1FF;
  uint64_t pt_index = (virt >> 12) & 0x1FF;
  uint64_t offset = virt & 0xFFF;
  uint64_t cr3;
  asm volatile ("mov %%cr3, %0": "=r"(cr3));
  uint64_t pml_phys = cr3 & ~0xFFFULL;
  uint64_t* pml4 = (uint64_t*) (pml_phys + hhdm_offset);
  uint64_t pml_entry = pml4[pml4_index];
  uint64_t pdpt;
  uint64_t pdpt_phys;
  if((pml_entry & 1) == 0){
    pdpt_phys = pmm_alloc_page();
    pdpt = pdpt_phys + hhdm_offset;
    memset((uint64_t*)pdpt, 0, 4096);
    pml4[pml4_index] = pml_entry |pdpt_phys| 1 | 2; //Set Present to 1 and writable to 1
  }
  else{
    pdpt_phys = pml_entry & ~0xFFFULL;
    pdpt = pdpt_phys + hhdm_offset;
  }
  uint64_t* pdpt_table = (uint64_t*) (pdpt_phys + hhdm_offset);
  uint64_t pdpt_entry = pdpt_table[pdpt_index];
  uint64_t pd;
  uint64_t pd_phys;
  if((pdpt_entry & 1) == 0){
    pd_phys = pmm_alloc_page();
    pd = pd_phys+hhdm_offset;
    memset((uint64_t*)pd, 0, 4096);
    pdpt_table[pdpt_index] = pdpt_entry | pd_phys | 1 | 2; // Look up a few lines im not going to explain the same thing over and over
  }else{
    pd_phys = pdpt_entry & ~0xFFFULL;
    pd = pd_phys + hhdm_offset;
  }

  uint64_t* pd_table = (uint64_t*)pd;
  uint64_t pd_entry = pd_table[pd_index];
  uint64_t pt;
  uint64_t pt_phys;
  if((pd_entry & 1) == 0){
    pt_phys = pmm_alloc_page();
    pt = pt_phys + hhdm_offset;
    memset((uint64_t*)pt, 0, 4096);
    pd_table[pd_index] = pd_entry | pt_phys | 1 | 2;
  }else{
    pt_phys = pd_entry & ~0xFFFULL;
    pt = pt_phys + hhdm_offset;
  }
 uint64_t* pt_table = (uint64_t*)pt;
 pt_table[pt_index] = physical | 1| 2;
}


void vmm_unmap(uint64_t virt){
  uint64_t hhdm_offset = hhdm_request.response->offset;
  uint64_t pml4_index = (virt >> 39) & 0x1FF;
  uint64_t pdpt_index = (virt >> 30) & 0x1FF;
  uint64_t pd_index = (virt >> 21) & 0x1FF;
  uint64_t pt_index = (virt >> 12) & 0x1FF;
  uint64_t offset = virt & 0xFFF;
  uint64_t cr3;
  asm volatile ("mov %%cr3, %0": "=r"(cr3));
  uint64_t pml_phys = cr3 & ~0xFFFULL;
  uint64_t* pml4 = (uint64_t*) (pml_phys + hhdm_offset);
  uint64_t pml_entry = pml4[pml4_index];
  if(!(pml_entry & 1)){
    return;
  }
  uint64_t pdpt_phys = pml_entry & ~0xFFFULL;
  uint64_t* pdpt = (uint64_t*)(pdpt_phys+hhdm_offset);

  uint64_t pdpt_entry = pdpt[pdpt_index];
  if(!(pdpt_entry & 1)){
    return;
  }
  uint64_t pd_phys = pdpt_entry & ~0xFFFULL;
  uint64_t* pd = (uint64_t*)(pd_phys + hhdm_offset);

  uint64_t pd_entry = pd[pd_index];
  if(!(pd_entry & 1)){
    return;
  }
  uint64_t pt_phys = pd_entry & ~0xFFFULL;
  uint64_t* pt = (uint64_t*)(pt_phys + hhdm_offset);
  uint64_t pte = pt[pt_index];
  if(!(pte & 1)){
    return;
  }

  pt[pt_index] = 0;
  asm volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}
