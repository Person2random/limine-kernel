#include "../../requests/requests.h"
#include "../pmm.h"
#include <stdint.h>
#include "valloc.h"
#include <stddef.h>
#include <stdbool.h>
#include "../../terminal/terminal.h"
#include "../../kstd/kstd.h"
valloc_range* valloc_head;
page_descriptor descriptors[128];
void valloc_init(){
  memset(descriptors,0,sizeof(page_descriptor)*128);
  uint64_t hhdm_offset = hhdm_request.response->offset;
  uint64_t max_phys = pmm_get_max_addr_phys();

  uint64_t end = hhdm_offset + max_phys;
  uint64_t page =  pmm_alloc_page();
  valloc_head = (struct valloc_range*)(page+hhdm_offset);
  valloc_range first_entry;
  first_entry.start = end;
  first_entry.length = 0x10000000000; //One terabyte of address space, Try making this 512 bytes one time and running the kernel again
  first_entry.next = 0;
  first_entry.free = true;
  
  *valloc_head = first_entry;
  descriptors[0].page_pointer = page;
  descriptors[0].used = 1;
}

valloc_range *new_range_descriptor() {
  uint64_t hhdm_offset = hhdm_request.response->offset;
  uint16_t pmm_slot = 6969;
  for(size_t i = 0; i < 128; i++){
    if(descriptors[i].used <128 && descriptors[i].page_pointer != 0){
      pmm_slot = i;
      break;
    }
  }
  if(pmm_slot == 6969){
    for(size_t i = 0; i < 128; i++){
      if(descriptors[i].page_pointer == 0){
        descriptors[i].page_pointer = pmm_alloc_page();
        pmm_slot = i;
        break;
      }       
    }
  }
  if (pmm_slot == 6969) {
    return 0;
  }
  uint16_t range_index = descriptors[pmm_slot].used;
  valloc_range *range_pointer =
    (valloc_range *)(
        descriptors[pmm_slot].page_pointer
        + hhdm_offset
        + (sizeof(valloc_range) * range_index)
    );
  descriptors[pmm_slot].used++;
  return range_pointer;
}

void *valloc(size_t pages) {
  uint64_t byte_size = pages * 4096; //Page size, 4 KiB, 0x1000
  valloc_range *current = valloc_head;
  uint64_t addr = 0;
  while(1){
    if(current->length >= byte_size && current->free == true){
      addr = current->start;
      break; 
    }
    if(current->next == 0) return 0; //No valid range left
    current = current->next;
  }
  uint64_t old_len = current->length;
  if(old_len - byte_size == 0){
    current->free = false;
    return (void*)addr;
  } 
  valloc_range* new = new_range_descriptor();
  if(new == 0){
    return 0;
  }
  current->length = byte_size;
  current->free = false;
  new->free = true;
  new->start = current->start + current->length;
  new->next = current->next;
  new->length = old_len - byte_size;
  current->next = new;
  return (void *)addr;
}

void vfree(void *ptr) {
  valloc_range* current = valloc_head;
  valloc_range* neighbor_left = 0;
  while (1) {
    if(current->start == (uint64_t)ptr){
      break;
    }
    if(current->next == 0){
      return;
    }
    neighbor_left = current;
    current = current->next;
  }
  current->free = true;
  valloc_range* neighbor_right = current->next;
  if(neighbor_right != 0 && neighbor_right->free == true ){
    current->length += neighbor_right->length;
    current->next = neighbor_right->next;
    neighbor_right->start = 0;
  }
  if (neighbor_left != 0 && neighbor_left->free == true) {
    neighbor_left->length += current->length;
    neighbor_left->next = current->next;
    current->start = 0;
    current = neighbor_left;
  }
}

void valloc_dump() {
  valloc_range* current = valloc_head;
  uint16_t index = 0;

  while (current != 0) {
    char buf[32];

    utoa(index, buf);
    debug_print(buf);

    if (current->free)
      debug_terminal_write("\tFree");
    else
      debug_terminal_write("\tNot free");

    debug_terminal_write("\tStart: ");
    utoa(current->start, buf);
    debug_print(buf);

    debug_terminal_write("\tLength: ");
    utoa(current->length, buf);
    debug_print(buf);

    debug_terminal_write("\tNext: ");
    utoa((uint64_t)current->next, buf);
    debug_print(buf);

    debug_terminal_write("\n");

    current = current->next;
    index++;
  }
}
